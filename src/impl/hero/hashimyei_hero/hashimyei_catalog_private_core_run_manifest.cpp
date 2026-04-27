bool parse_latent_lineage_state_payload(
    std::string_view payload,
    std::unordered_map<std::string, std::string> *out) {
  if (!out)
    return false;
  out->clear();
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos)
      line_end = payload.size();
    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r')
      line.remove_suffix(1);
    const std::string trimmed = trim_ascii(line);
    if (!trimmed.empty() && trimmed.front() != '#') {
      const std::size_t eq = trimmed.find('=');
      if (eq != std::string_view::npos && eq > 0) {
        const std::string key =
            cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
                trimmed.substr(0, eq));
        const std::string val = trim_ascii(trimmed.substr(eq + 1));
        if (!key.empty())
          (*out)[key] = val;
      }
    }
    if (line_end == payload.size())
      break;
    cursor = line_end + 1;
  }
  return true;
}

std::string
compute_run_id(const cuwacunu::hashimyei::hashimyei_t &campaign_identity,
               const wave_contract_binding_t &wave_contract_binding,
               std::uint64_t started_at_ms) {
  cuwacunu::hashimyei::hashimyei_t normalized_campaign = campaign_identity;
  normalize_identity_hex_name_inplace(&normalized_campaign);
  wave_contract_binding_t normalized_binding = wave_contract_binding;
  normalize_identity_hex_name_inplace(&normalized_binding.identity);
  normalize_identity_hex_name_inplace(&normalized_binding.contract);
  normalize_identity_hex_name_inplace(&normalized_binding.wave);
  std::ostringstream seed;
  seed << normalized_campaign.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(
              normalized_campaign.kind)
       << "|" << normalized_campaign.name << "|" << normalized_campaign.ordinal
       << "|" << normalized_campaign.hash_sha256_hex << "|"
       << normalized_binding.identity.schema << "|"
       << cuwacunu::hashimyei::hashimyei_kind_to_string(
              normalized_binding.identity.kind)
       << "|" << normalized_binding.identity.name << "|"
       << normalized_binding.identity.ordinal << "|"
       << normalized_binding.identity.hash_sha256_hex << "|"
       << normalized_binding.contract.hash_sha256_hex << "|"
       << normalized_binding.wave.hash_sha256_hex << "|"
       << normalized_binding.binding_id << "|" << started_at_ms;
  std::string hash_hex;
  if (!sha256_hex_bytes(seed.str(), &hash_hex) || hash_hex.empty()) {
    return "run.invalid";
  }
  return "run." + hash_hex.substr(0, 24);
}

bool load_run_manifest(const std::filesystem::path &path, run_manifest_t *out,
                       std::string *error) {
  clear_error(error);
  if (!out) {
    set_error(error, "run manifest output pointer is null");
    return false;
  }
  *out = run_manifest_t{};

  if (path.filename() == "run.manifest.v1.kv") {
    set_error(error,
              std::string("v1 run manifest filename is not supported; use ") +
                  cuwacunu::hashimyei::kRunManifestFilenameV2);
    return false;
  }
  if (path.filename() != cuwacunu::hashimyei::kRunManifestFilenameV2) {
    set_error(error, std::string("invalid run manifest filename, expected ") +
                         cuwacunu::hashimyei::kRunManifestFilenameV2);
    return false;
  }

  std::string payload;
  std::string read_error;
  if (!read_text_file(path, &payload, &read_error)) {
    set_error(error, "cannot read run manifest: " + read_error);
    return false;
  }

  std::unordered_map<std::string, std::string> kv;
  (void)parse_latent_lineage_state_payload(payload, &kv);
  out->schema = kv.count("schema") != 0
                    ? trim_ascii(kv.at("schema"))
                    : cuwacunu::hashimyei::kRunManifestSchemaV2;
  if (out->schema != cuwacunu::hashimyei::kRunManifestSchemaV2) {
    set_error(error, std::string("invalid run manifest schema; expected ") +
                         cuwacunu::hashimyei::kRunManifestSchemaV2);
    return false;
  }
  out->run_id = kv["run_id"];
  (void)parse_u64(kv["started_at_ms"], &out->started_at_ms);
  if (!parse_identity_kv(kv, "campaign_identity", true, &out->campaign_identity,
                         error)) {
    return false;
  }
  if (!parse_binding_kv(kv, "wave_contract_binding",
                        &out->wave_contract_binding, error)) {
    return false;
  }
  out->sampler = kv["sampler"];
  out->record_type = kv["record_type"];
  out->seed = kv["seed"];
  out->device = kv["device"];
  out->dtype = kv["dtype"];

  std::uint64_t dependency_count = 0;
  if (parse_u64(kv["dependency_count"], &dependency_count)) {
    out->dependency_files.reserve(static_cast<std::size_t>(dependency_count));
    for (std::uint64_t i = 0; i < dependency_count; ++i) {
      std::ostringstream pfx;
      pfx << "dependency_" << std::setw(4) << std::setfill('0') << i << "_";
      dependency_file_t d{};
      d.canonical_path = kv[pfx.str() + "path"];
      d.sha256_hex = kv[pfx.str() + "sha256"];
      if (!d.canonical_path.empty() || !d.sha256_hex.empty()) {
        out->dependency_files.push_back(std::move(d));
      }
    }
  }

  std::uint64_t component_count = 0;
  if (parse_u64(kv["component_count"], &component_count)) {
    out->components.reserve(static_cast<std::size_t>(component_count));
    for (std::uint64_t i = 0; i < component_count; ++i) {
      std::ostringstream pfx;
      pfx << "component_" << std::setw(4) << std::setfill('0') << i << "_";
      component_instance_t c{};
      c.canonical_path = kv[pfx.str() + "canonical_path"];
      c.family = kv[pfx.str() + "family"];
      c.hashimyei = kv[pfx.str() + "hashimyei"];
      if (!c.canonical_path.empty() || !c.family.empty() ||
          !c.hashimyei.empty()) {
        out->components.push_back(std::move(c));
      }
    }
  }

  if (out->schema.empty() || out->run_id.empty()) {
    set_error(error, "invalid run manifest: missing schema or run_id");
    return false;
  }
  normalize_run_manifest_inplace(out);
  return true;
}
