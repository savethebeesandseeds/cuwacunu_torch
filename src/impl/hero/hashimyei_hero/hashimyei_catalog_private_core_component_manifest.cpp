bool parse_component_manifest_kv(
    const std::unordered_map<std::string, std::string> &kv,
    component_manifest_t *out, std::string *error) {
  clear_error(error);
  if (!out) {
    set_error(error, "component manifest output pointer is null");
    return false;
  }

  *out = component_manifest_t{};
  out->schema = kv.count("schema") != 0
                    ? trim_ascii(kv.at("schema"))
                    : cuwacunu::hashimyei::kComponentManifestSchemaV2;
  out->canonical_path =
      kv.count("canonical_path") != 0
          ? normalize_hashimyei_canonical_path_or_same(kv.at("canonical_path"))
          : std::string{};
  out->family = kv.count("family") != 0 ? kv.at("family") : std::string{};
  out->component_tag =
      kv.count("component_tag") != 0 ? kv.at("component_tag") : std::string{};
  if (!parse_identity_kv(kv, "hashimyei_identity", true,
                         &out->hashimyei_identity, error)) {
    return false;
  }
  if (!parse_identity_kv(kv, "contract_identity", true, &out->contract_identity,
                         error)) {
    return false;
  }
  bool parent_present = false;
  if (const auto it = kv.find("parent_identity.present"); it != kv.end()) {
    const std::string token = lowercase_copy(trim_ascii(it->second));
    parent_present = token == "1" || token == "true" || token == "yes";
  } else {
    parent_present = kv.count("parent_identity.name") != 0;
  }
  if (parent_present) {
    cuwacunu::hashimyei::hashimyei_t parent{};
    if (!parse_identity_kv(kv, "parent_identity", true, &parent, error))
      return false;
    out->parent_identity = parent;
  }
  out->revision_reason =
      kv.count("revision_reason") != 0
          ? kv.at("revision_reason")
          : (out->parent_identity.has_value() ? "dsl_change" : "initial");
  out->founding_revision_id = kv.count("founding_revision_id") != 0
                                  ? kv.at("founding_revision_id")
                                  : std::string{};
  out->founding_dsl_source_path = kv.count("founding_dsl_source_path") != 0
                                      ? kv.at("founding_dsl_source_path")
                                      : std::string{};
  out->founding_dsl_source_sha256_hex =
      kv.count("founding_dsl_source_sha256_hex") != 0
          ? kv.at("founding_dsl_source_sha256_hex")
          : std::string{};
  out->docking_signature_sha256_hex =
      kv.count("docking_signature_sha256_hex") != 0
          ? kv.at("docking_signature_sha256_hex")
          : std::string{};
  out->component_compatibility_sha256_hex =
      kv.count("component_compatibility_sha256_hex") != 0
          ? kv.at("component_compatibility_sha256_hex")
          : std::string{};
  for (const auto &field : cuwacunu::camahjucunu::instrument_signature_fields(
           out->instrument_signature)) {
    const std::string key = "instrument_signature." + field.first;
    if (const auto it = kv.find(key); it != kv.end()) {
      std::string field_error{};
      if (!cuwacunu::camahjucunu::instrument_signature_set_field(
              &out->instrument_signature, field.first, it->second,
              &field_error)) {
        set_error(error, field_error);
        return false;
      }
    }
  }
  out->lineage_state = kv.count("lineage_state") != 0 ? kv.at("lineage_state")
                                                      : std::string{"active"};
  out->replaced_by =
      kv.count("replaced_by") != 0 ? kv.at("replaced_by") : std::string{};
  (void)parse_u64(kv.count("created_at_ms") != 0 ? kv.at("created_at_ms") : "0",
                  &out->created_at_ms);
  (void)parse_u64(kv.count("updated_at_ms") != 0 ? kv.at("updated_at_ms") : "0",
                  &out->updated_at_ms);
  if (out->family.empty()) {
    out->family = component_family_from_parts(out->canonical_path,
                                              out->hashimyei_identity.name);
  }

  normalize_component_manifest_inplace(out);
  return validate_component_manifest(*out, error);
}

bool validate_component_manifest(const component_manifest_t &manifest,
                                 std::string *error) {
  clear_error(error);
  const auto starts_with = [](std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() &&
           text.substr(0, prefix.size()) == prefix;
  };
  const auto is_hex_64 = [](std::string_view s) {
    if (s.size() != 64)
      return false;
    for (char c : s) {
      const bool digit = c >= '0' && c <= '9';
      const bool lower = c >= 'a' && c <= 'f';
      const bool upper = c >= 'A' && c <= 'F';
      if (!digit && !lower && !upper)
        return false;
    }
    return true;
  };

  if (manifest.schema != cuwacunu::hashimyei::kComponentManifestSchemaV2) {
    set_error(error, "invalid component manifest schema");
    return false;
  }
  if (manifest.canonical_path.empty() ||
      !starts_with(manifest.canonical_path, "tsi.")) {
    set_error(error, "component manifest missing canonical_path");
    return false;
  }
  if (manifest.family.empty() || !starts_with(manifest.family, "tsi.")) {
    set_error(error, "component manifest missing family");
    return false;
  }
  if (!is_valid_component_tag_token(trim_ascii(manifest.component_tag))) {
    set_error(error, "component manifest missing or invalid component_tag");
    return false;
  }
  std::string identity_error;
  if (!cuwacunu::hashimyei::validate_hashimyei(manifest.hashimyei_identity,
                                               &identity_error)) {
    set_error(error, "component manifest hashimyei_identity invalid: " +
                         identity_error);
    return false;
  }
  if (manifest.hashimyei_identity.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE) {
    set_error(error,
              "component manifest hashimyei_identity.kind must be TSIEMENE");
    return false;
  }
  if (!cuwacunu::hashimyei::validate_hashimyei(manifest.contract_identity,
                                               &identity_error)) {
    set_error(error, "component manifest contract_identity invalid: " +
                         identity_error);
    return false;
  }
  if (manifest.contract_identity.kind !=
      cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT) {
    set_error(error,
              "component manifest contract_identity.kind must be CONTRACT");
    return false;
  }
  if (manifest.parent_identity.has_value()) {
    if (!cuwacunu::hashimyei::validate_hashimyei(*manifest.parent_identity,
                                                 &identity_error)) {
      set_error(error, "component manifest parent_identity invalid: " +
                           identity_error);
      return false;
    }
    if (manifest.parent_identity->kind !=
        cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE) {
      set_error(error,
                "component manifest parent_identity.kind must be TSIEMENE");
      return false;
    }
  }
  if (trim_ascii(manifest.revision_reason).empty()) {
    set_error(error, "component manifest missing revision_reason");
    return false;
  }
  if (manifest.founding_dsl_source_path.empty()) {
    set_error(error, "component manifest missing founding_dsl_source_path");
    return false;
  }
  if (!is_hex_64(manifest.founding_dsl_source_sha256_hex)) {
    set_error(error,
              "component manifest founding_dsl_source_sha256_hex must be 64 "
              "hex chars");
    return false;
  }
  if (!trim_ascii(manifest.docking_signature_sha256_hex).empty() &&
      !is_hex_64(manifest.docking_signature_sha256_hex)) {
    set_error(error,
              "component manifest docking_signature_sha256_hex must be 64 "
              "hex chars when present");
    return false;
  }
  if (!is_hex_64(manifest.component_compatibility_sha256_hex)) {
    set_error(error,
              "component manifest component_compatibility_sha256_hex must be "
              "64 hex chars");
    return false;
  }
  std::string signature_error{};
  if (!cuwacunu::camahjucunu::instrument_signature_validate(
          manifest.instrument_signature, /*allow_any=*/true,
          "component manifest instrument_signature", &signature_error)) {
    set_error(error, signature_error);
    return false;
  }

  const std::string lineage_state = trim_ascii(manifest.lineage_state);
  if (lineage_state != "active" && lineage_state != "deprecated" &&
      lineage_state != "replaced" && lineage_state != "tombstone") {
    set_error(error, "component manifest lineage_state must be "
                     "active/deprecated/replaced/tombstone");
    return false;
  }
  if (lineage_state == "replaced" && manifest.replaced_by.empty()) {
    set_error(error,
              "component manifest lineage_state=replaced requires replaced_by");
    return false;
  }
  if (manifest.created_at_ms == 0) {
    set_error(error, "component manifest missing created_at_ms");
    return false;
  }
  if (manifest.updated_at_ms != 0 &&
      manifest.updated_at_ms < manifest.created_at_ms) {
    set_error(error,
              "component manifest updated_at_ms is older than created_at_ms");
    return false;
  }
  return true;
}

bool load_component_manifest(const std::filesystem::path &path,
                             component_manifest_t *out, std::string *error) {
  clear_error(error);
  if (!out) {
    set_error(error, "component manifest output pointer is null");
    return false;
  }
  *out = component_manifest_t{};

  if (path.filename() == "component.manifest.v1.kv") {
    set_error(
        error,
        std::string("v1 component manifest filename is not supported; use ") +
            cuwacunu::hashimyei::kComponentManifestFilenameV2);
    return false;
  }
  if (path.filename() != cuwacunu::hashimyei::kComponentManifestFilenameV2) {
    set_error(error,
              std::string("invalid component manifest filename, expected ") +
                  cuwacunu::hashimyei::kComponentManifestFilenameV2);
    return false;
  }

  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read component manifest: " + read_error);
    return false;
  }
  if (payload.empty()) {
    set_error(error, "component manifest payload is empty");
    return false;
  }

  std::unordered_map<std::string, std::string> kv;
  (void)parse_latent_lineage_state_payload(payload, &kv);
  if (!parse_component_manifest_kv(kv, out, error))
    return false;
  return true;
}

std::string
compute_component_manifest_id(const component_manifest_t &manifest) {
  component_manifest_t normalized = manifest;
  normalize_component_manifest_inplace(&normalized);
  std::ostringstream seed;
  seed << normalized.canonical_path << "|" << normalized.family << "|"
       << normalized.component_tag << "|"
       << normalized.hashimyei_identity.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(
              normalized.hashimyei_identity.kind)
       << "|" << normalized.hashimyei_identity.name << "|"
       << normalized.hashimyei_identity.ordinal << "|"
       << normalized.hashimyei_identity.hash_sha256_hex << "|";
  if (normalized.parent_identity.has_value()) {
    seed << normalized.parent_identity->schema << "|"
         << cuwacunu::hashimyei::hashimyei_kind_to_string(
                normalized.parent_identity->kind)
         << "|" << normalized.parent_identity->name << "|"
         << normalized.parent_identity->ordinal << "|"
         << normalized.parent_identity->hash_sha256_hex << "|";
  }
  seed << normalized.revision_reason << "|" << normalized.founding_revision_id
       << "|" << normalized.contract_identity.hash_sha256_hex << "|"
       << normalized.founding_dsl_source_sha256_hex << "|"
       << normalized.docking_signature_sha256_hex << "|"
       << normalized.component_compatibility_sha256_hex << "|"
       << cuwacunu::camahjucunu::instrument_signature_compact_string(
              normalized.instrument_signature);
  std::string digest;
  if (!sha256_hex_bytes(seed.str(), &digest) || digest.empty()) {
    return "component.invalid";
  }
  return "component." + digest.substr(0, 24);
}
