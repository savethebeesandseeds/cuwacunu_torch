[[nodiscard]] std::filesystem::path
store_root_from_catalog_path(const std::filesystem::path &catalog_path) {
  const std::filesystem::path catalog_dir = catalog_path.parent_path();
  if (catalog_dir.filename() == "catalog" &&
      catalog_dir.parent_path().filename() ==
          std::string(cuwacunu::hashimyei::kHashimyeiMetaDirname)) {
    return catalog_dir.parent_path().parent_path();
  }
  return catalog_dir;
}

std::string compute_founding_dsl_bundle_aggregate_sha256(
    const founding_dsl_bundle_manifest_t &manifest) {
  std::ostringstream seed;
  seed << manifest.schema << "|" << manifest.component_id << "|"
       << manifest.canonical_path << "|" << manifest.hashimyei_name << "|"
       << manifest.files.size() << "|";
  for (const auto &file : manifest.files) {
    seed << file.snapshot_relpath << "|" << file.sha256_hex << "|";
  }
  std::string digest{};
  if (!sha256_hex_bytes(seed.str(), &digest))
    return {};
  return digest;
}

bool write_founding_dsl_bundle_manifest(
    const std::filesystem::path &store_root,
    const founding_dsl_bundle_manifest_t &manifest, std::string *error) {
  if (error)
    error->clear();
  if (trim_ascii(manifest.component_id).empty()) {
    if (error)
      *error = "founding DSL bundle manifest missing component_id";
    return false;
  }

  const auto manifest_path = founding_dsl_bundle_manifest_path(
      store_root, manifest.canonical_path, manifest.component_id);
  std::error_code ec{};
  std::filesystem::create_directories(manifest_path.parent_path(), ec);
  if (ec) {
    if (error) {
      *error = "cannot create founding DSL bundle directory: " +
               manifest_path.parent_path().string();
    }
    return false;
  }

  std::ofstream out(manifest_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) {
      *error = "cannot open founding DSL bundle manifest for write: " +
               manifest_path.string();
    }
    return false;
  }

  out << "schema=" << cuwacunu::hashimyei::kFoundingDslBundleManifestSchemaV1
      << "\n";
  out << "component_id=" << manifest.component_id << "\n";
  out << "canonical_path=" << manifest.canonical_path << "\n";
  out << "hashimyei_name=" << manifest.hashimyei_name << "\n";
  out << "founding_dsl_bundle_aggregate_sha256_hex="
      << manifest.founding_dsl_bundle_aggregate_sha256_hex << "\n";
  out << "file_count=" << manifest.files.size() << "\n";
  for (std::size_t i = 0; i < manifest.files.size(); ++i) {
    out << founding_bundle_file_key(i, "source_path") << "="
        << manifest.files[i].source_path << "\n";
    out << founding_bundle_file_key(i, "snapshot_relpath") << "="
        << manifest.files[i].snapshot_relpath << "\n";
    out << founding_bundle_file_key(i, "sha256_hex") << "="
        << manifest.files[i].sha256_hex << "\n";
  }
  if (!out) {
    if (error) {
      *error = "cannot write founding DSL bundle manifest: " +
               manifest_path.string();
    }
    return false;
  }
  return true;
}

bool read_founding_dsl_bundle_manifest(const std::filesystem::path &store_root,
                                       std::string_view canonical_path,
                                       std::string_view component_id,
                                       founding_dsl_bundle_manifest_t *out,
                                       std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error) {
      *error = "founding DSL bundle manifest output pointer is null";
    }
    return false;
  }
  *out = founding_dsl_bundle_manifest_t{};

  const auto manifest_path = founding_dsl_bundle_manifest_path(
      store_root, canonical_path, component_id);
  std::ifstream in(manifest_path, std::ios::binary);
  if (!in) {
    if (error) {
      *error =
          "founding DSL bundle manifest not found: " + manifest_path.string();
    }
    return false;
  }

  std::unordered_map<std::string, std::string> kv{};
  std::string line{};
  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    const std::string key = trim_ascii(line.substr(0, eq));
    const std::string value = line.substr(eq + 1);
    if (!key.empty())
      kv[key] = value;
  }
  if (!in.eof() && in.fail()) {
    if (error) {
      *error = "cannot parse founding DSL bundle manifest: " +
               manifest_path.string();
    }
    return false;
  }

  if (const auto it = kv.find("schema"); it != kv.end())
    out->schema = it->second;
  if (out->schema != cuwacunu::hashimyei::kFoundingDslBundleManifestSchemaV1) {
    if (error) {
      *error =
          "unsupported founding DSL bundle manifest schema: " + out->schema;
    }
    return false;
  }
  if (const auto it = kv.find("component_id"); it != kv.end()) {
    out->component_id = it->second;
  }
  if (const auto it = kv.find("canonical_path"); it != kv.end()) {
    out->canonical_path = it->second;
  }
  if (const auto it = kv.find("hashimyei_name"); it != kv.end()) {
    out->hashimyei_name = it->second;
  }
  if (const auto it = kv.find("founding_dsl_bundle_aggregate_sha256_hex");
      it != kv.end()) {
    out->founding_dsl_bundle_aggregate_sha256_hex = it->second;
  }
  const auto count_it = kv.find("file_count");
  if (count_it == kv.end()) {
    if (error)
      *error = "founding DSL bundle manifest missing file_count";
    return false;
  }
  std::size_t file_count = 0;
  try {
    file_count =
        static_cast<std::size_t>(std::stoull(trim_ascii(count_it->second)));
  } catch (...) {
    if (error)
      *error = "founding DSL bundle manifest has invalid file_count";
    return false;
  }
  out->files.clear();
  out->files.reserve(file_count);
  for (std::size_t i = 0; i < file_count; ++i) {
    founding_dsl_bundle_manifest_file_t file{};
    if (const auto it = kv.find(founding_bundle_file_key(i, "source_path"));
        it != kv.end()) {
      file.source_path = it->second;
    }
    if (const auto it =
            kv.find(founding_bundle_file_key(i, "snapshot_relpath"));
        it != kv.end()) {
      file.snapshot_relpath = it->second;
    }
    if (const auto it = kv.find(founding_bundle_file_key(i, "sha256_hex"));
        it != kv.end()) {
      file.sha256_hex = it->second;
    }
    out->files.push_back(std::move(file));
  }
  return true;
}
