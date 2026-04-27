namespace {

[[nodiscard]] std::string
kind_sha_key(cuwacunu::hashimyei::hashimyei_kind_e kind,
             std::string_view sha256_hex) {
  return cuwacunu::hashimyei::hashimyei_kind_to_string(kind) + "|" +
         std::string(sha256_hex);
}

[[nodiscard]] bool is_known_schema(std::string_view schema) {
  if (schema == cuwacunu::hero::family_rank::kFamilyRankSchemaV3)
    return true;
  if (schema == "wave.source.runtime.projection.v2")
    return true;
  if (schema == "piaabo.torch_compat.data_analytics.v2")
    return true;
  if (schema == "piaabo.torch_compat.data_analytics_symbolic.v2")
    return true;
  if (schema == "piaabo.torch_compat.embedding_sequence_analytics.v2")
    return true;
  if (schema ==
      "piaabo.torch_compat.embedding_sequence_analytics_symbolic.v2") {
    return true;
  }
  if (schema == "tsi.wikimyei.representation.encoding.vicreg.status.v1")
    return true;
  if (schema == "tsi.wikimyei.representation.encoding.vicreg.transfer_matrix_"
                "evaluation.run.v1") {
    return true;
  }
  if (schema == "piaabo.torch_compat.network_analytics.v5") {
    return true;
  }
  return false;
}

[[nodiscard]] bool parse_double_token(std::string_view in, double *out) {
  if (!out)
    return false;
  const std::string t = trim_ascii(in);
  if (t.empty())
    return false;
  std::size_t pos = 0;
  double v = 0.0;
  try {
    v = std::stod(t, &pos);
  } catch (...) {
    return false;
  }
  if (pos != t.size())
    return false;
  if (!std::isfinite(v))
    return false;
  *out = v;
  return true;
}

[[nodiscard]] std::string
maybe_hashimyei_from_canonical(std::string_view canonical) {
  const std::size_t dot = canonical.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= canonical.size())
    return {};
  return normalize_hashimyei_name_or_same(canonical.substr(dot + 1));
}

[[nodiscard]] std::string
component_family_from_parts(std::string_view canonical_path,
                            std::string_view hashimyei) {
  const std::string normalized_canonical =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  const std::string normalized_hash =
      normalize_hashimyei_name_or_same(hashimyei);
  if (!hashimyei.empty() && !canonical_path.empty()) {
    std::string family = normalized_canonical;
    const std::string suffix = "." + normalized_hash;
    if (family.size() > suffix.size() &&
        family.compare(family.size() - suffix.size(), suffix.size(), suffix) ==
            0) {
      family.resize(family.size() - suffix.size());
    }
    if (!family.empty())
      return family;
  }
  if (!normalized_canonical.empty()) {
    const std::string family = normalized_canonical;
    if (!family.empty()) {
      return family;
    }
  }
  return {};
}

[[nodiscard]] std::string component_active_pointer_key(
    std::string_view canonical_path, std::string_view family,
    std::string_view hashimyei, std::string_view contract_hash) {
  const std::string normalized_hash =
      normalize_hashimyei_name_or_same(hashimyei);
  std::string canonical_key_path =
      normalize_hashimyei_canonical_path_or_same(canonical_path);
  if (!normalized_hash.empty()) {
    canonical_key_path =
        component_family_from_parts(canonical_key_path, normalized_hash);
  }
  const std::string family_key =
      family.empty()
          ? component_family_from_parts(canonical_key_path, normalized_hash)
          : normalize_hashimyei_canonical_path_or_same(family);
  return canonical_key_path + "|" + family_key + "|" +
         trim_ascii(contract_hash);
}

[[nodiscard]] bool parse_component_active_pointer_key(
    std::string_view key, std::string *out_canonical_path,
    std::string *out_family, std::string *out_contract_hash) {
  const std::size_t first_sep = key.find('|');
  if (first_sep == std::string::npos)
    return false;
  const std::size_t second_sep = key.find('|', first_sep + 1);
  if (second_sep == std::string::npos)
    return false;
  if (out_canonical_path) {
    *out_canonical_path = std::string(key.substr(0, first_sep));
  }
  if (out_family) {
    *out_family =
        std::string(key.substr(first_sep + 1, second_sep - first_sep - 1));
  }
  if (out_contract_hash) {
    *out_contract_hash = std::string(key.substr(second_sep + 1));
  }
  return true;
}

[[nodiscard]] bool is_family_rank_schema(std::string_view schema) {
  return schema == cuwacunu::hero::family_rank::kFamilyRankSchemaV3;
}

[[nodiscard]] std::string
family_rank_record_id(std::string_view family,
                      std::string_view component_compatibility_sha256_hex,
                      std::string_view artifact_sha256) {
  return cuwacunu::hero::family_rank::scope_key(
             family, component_compatibility_sha256_hex) +
         "|" + std::string(artifact_sha256);
}

[[nodiscard]] std::optional<std::filesystem::path>
canonicalized(const std::filesystem::path &p) {
  std::error_code ec;
  const auto c = std::filesystem::weakly_canonical(p, ec);
  if (ec)
    return std::nullopt;
  return c;
}

[[nodiscard]] bool path_is_within(std::filesystem::path base,
                                  std::filesystem::path candidate) {
  base = base.lexically_normal();
  candidate = candidate.lexically_normal();
  auto b = base.begin();
  auto c = candidate.begin();
  for (; b != base.end() && c != candidate.end(); ++b, ++c) {
    if (*b != *c)
      return false;
  }
  return b == base.end();
}

[[nodiscard]] std::string
canonical_path_from_report_fragment_path(const std::filesystem::path &p) {
  const std::string s = p.generic_string();
  std::size_t begin = s.find("/tsi/");
  begin = (begin == std::string::npos) ? s.find("tsi/") : begin + 1;
  if (begin == std::string::npos)
    return {};

  std::vector<std::string> parts{};
  std::size_t pos = begin;
  while (pos <= s.size()) {
    const std::size_t slash = s.find('/', pos);
    if (slash == std::string::npos) {
      parts.emplace_back(s.substr(pos));
      break;
    }
    parts.emplace_back(s.substr(pos, slash - pos));
    pos = slash + 1;
  }

  if (parts.size() >= 5 && parts[0] == "tsi" && parts[1] == "wikimyei" &&
      parts[2] == "representation" && parts[3] == "vicreg" &&
      !parts[4].empty() && parts[4].rfind("0x", 0) == 0) {
    return "tsi.wikimyei.representation.encoding.vicreg." + parts[4];
  }

  if (parts.size() >= 3 && parts[0] == "tsi" && parts[1] == "source" &&
      parts[2] == "dataloader") {
    return "tsi.source.dataloader";
  }
  return {};
}

[[nodiscard]] std::string
contract_hash_from_report_fragment_path(const std::filesystem::path &p) {
  const std::string s = p.generic_string();
  std::size_t begin = s.find("/tsi/");
  begin = (begin == std::string::npos) ? s.find("tsi/") : begin + 1;
  if (begin == std::string::npos)
    return {};

  std::vector<std::string> parts{};
  std::size_t pos = begin;
  while (pos <= s.size()) {
    const std::size_t slash = s.find('/', pos);
    if (slash == std::string::npos) {
      parts.emplace_back(s.substr(pos));
      break;
    }
    parts.emplace_back(s.substr(pos, slash - pos));
    pos = slash + 1;
  }
  for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
    if (parts[i] == "contracts") {
      const std::string token = trim_ascii(parts[i + 1]);
      if (token.rfind("c.", 0) == 0)
        return {};
      return token;
    }
  }
  return {};
}

[[nodiscard]] std::string as_text_or_empty(idydb **db,
                                           idydb_column_row_sizing column,
                                           idydb_column_row_sizing row) {
  if (idydb_extract(db, column, row) != IDYDB_DONE)
    return {};
  if (idydb_retrieved_type(db) != IDYDB_CHAR)
    return {};
  const char *s = idydb_retrieve_char(db);
  return s ? std::string(s) : std::string{};
}

[[nodiscard]] std::optional<double>
as_numeric_or_null(idydb **db, idydb_column_row_sizing column,
                   idydb_column_row_sizing row) {
  if (idydb_extract(db, column, row) != IDYDB_DONE)
    return std::nullopt;
  if (idydb_retrieved_type(db) != IDYDB_FLOAT)
    return std::nullopt;
  return static_cast<double>(idydb_retrieve_float(db));
}

void clear_error(std::string *error) {
  if (error)
    error->clear();
}

void set_error(std::string *error, std::string_view msg) {
  if (error)
    *error = std::string(msg);
}

[[nodiscard]] std::string
compact_lock_owner_description(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  bool pending_separator = false;
  for (char c : text) {
    if (c == '\n' || c == '\r') {
      pending_separator = !out.empty();
      continue;
    }
    if (pending_separator) {
      if (!out.empty() && out.back() != ' ' && out.back() != ';')
        out += "; ";
      pending_separator = false;
    }
    out.push_back(c);
  }
  return trim_ascii(out);
}

[[nodiscard]] const char *idydb_rc_label(int rc) {
  switch (rc) {
  case IDYDB_SUCCESS:
    return "success";
  case IDYDB_ERROR:
    return "error";
  case IDYDB_PERM:
    return "permission_denied";
  case IDYDB_BUSY:
    return "busy";
  case IDYDB_NOT_FOUND:
    return "not_found";
  case IDYDB_CORRUPT:
    return "corrupt";
  case IDYDB_RANGE:
    return "range";
  case IDYDB_DONE:
    return "done";
  default:
    return "unknown";
  }
}

constexpr int kCatalogOpenBusyRetryCount = 40;
constexpr auto kCatalogOpenBusyRetryDelay = std::chrono::milliseconds(25);

[[nodiscard]] bool insert_text(idydb **db, idydb_column_row_sizing col,
                               idydb_column_row_sizing row,
                               std::string_view value, std::string *error) {
  if (value.empty())
    return true;
  const std::string v(value);
  const int rc = idydb_insert_const_char(db, col, row, v.c_str());
  if (rc == IDYDB_DONE)
    return true;
  if (error) {
    const char *msg = idydb_errmsg(db);
    *error = "idydb_insert_const_char failed: " +
             std::string(msg ? msg : "<no error message>");
  }
  return false;
}

[[nodiscard]] bool insert_num(idydb **db, idydb_column_row_sizing col,
                              idydb_column_row_sizing row, double value,
                              std::string *error) {
  if (!std::isfinite(value))
    return true;
  const int rc = idydb_insert_float(db, col, row, static_cast<float>(value));
  if (rc == IDYDB_DONE)
    return true;
  if (error) {
    const char *msg = idydb_errmsg(db);
    *error = "idydb_insert_float failed: " +
             std::string(msg ? msg : "<no error message>");
  }
  return false;
}

[[nodiscard]] std::string join_key(std::string_view canonical_path,
                                   std::string_view schema) {
  return std::string(canonical_path) + "|" + std::string(schema);
}

[[nodiscard]] bool parse_runtime_lls_payload(
    std::string_view payload, std::unordered_map<std::string, std::string> *out,
    runtime_lls_document_t *out_document, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "runtime .lls kv output pointer is null";
    return false;
  }
  out->clear();
  if (out_document)
    out_document->entries.clear();
  if (!out_document) {
    return cuwacunu::piaabo::latent_lineage_state::
        parse_runtime_lls_text_fast_to_kv_map(payload, out, error);
  }

  runtime_lls_document_t parsed{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &parsed, error)) {
    return false;
  }
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          parsed, out, error)) {
    return false;
  }
  if (const auto it = out->find("schema");
      it == out->end() || it->second.empty()) {
    if (error)
      *error = "persisted runtime .lls requires top-level schema key";
    return false;
  }
  if (out_document)
    *out_document = std::move(parsed);
  return true;
}

void populate_runtime_report_entry_header_fields_(
    const std::unordered_map<std::string, std::string> &kv,
    report_fragment_entry_t *entry) {
  if (!entry)
    return;
  if (entry->source_label.empty()) {
    if (const auto it = kv.find("source_label"); it != kv.end()) {
      entry->source_label = trim_ascii(it->second);
    }
  }
  cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t header{};
  if (!cuwacunu::piaabo::latent_lineage_state::
          parse_runtime_report_header_from_kv(kv, &header, nullptr)) {
    return;
  }
  if (entry->semantic_taxon.empty()) {
    entry->semantic_taxon = trim_ascii(header.semantic_taxon);
  }
  if (entry->report_canonical_path.empty()) {
    entry->report_canonical_path = trim_ascii(header.context.canonical_path);
  }
  if (entry->binding_id.empty()) {
    entry->binding_id = trim_ascii(header.context.binding_id);
  }
  if (entry->source_runtime_cursor.empty()) {
    entry->source_runtime_cursor =
        trim_ascii(header.context.source_runtime_cursor);
  }
  if (!entry->has_wave_cursor && header.context.has_wave_cursor) {
    entry->has_wave_cursor = true;
    entry->wave_cursor = header.context.wave_cursor;
  }
}

} // namespace
