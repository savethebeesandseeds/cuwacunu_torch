std::string sequence_analytics_to_latent_lineage_state_text(
    const sequence_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view sequence_label,
    const tsiemene::component_report_identity_t& report_identity) {
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      make_sequence_runtime_lls_document_(
          report,
          options,
          generic_sequence_report_keys_(),
          sequence_label,
          report_identity));
}

std::string data_analytics_to_latent_lineage_state_text(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view source_label,
    const tsiemene::component_report_identity_t& report_identity,
    std::string_view contract_hash) {
  auto sequence_report = make_sequence_analytics_report(report);
  sequence_report.schema = report.schema;
  auto document = make_sequence_runtime_lls_document_(
      sequence_report,
      options,
      source_sequence_report_keys_(),
      source_label,
      report_identity);
  const std::string_view normalized_contract_hash =
      trim_ascii_ws_view_(contract_hash);
  if (!normalized_contract_hash.empty()) {
    document.entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "contract_hash", std::string(normalized_contract_hash)));
  }
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      document);
}

std::string sequence_symbolic_analytics_to_latent_lineage_state_text(
    const sequence_symbolic_analytics_report_t& report,
    std::string_view sequence_label,
    const tsiemene::component_report_identity_t& report_identity,
    sequence_symbolic_report_compaction_options_t compaction_options) {
  const auto compacted =
      compact_sequence_symbolic_analytics_report(report, compaction_options);
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      make_symbolic_runtime_lls_document_(
          compacted,
          generic_symbolic_report_keys_(),
          sequence_label,
          report_identity));
}

std::string data_symbolic_analytics_to_latent_lineage_state_text(
    const data_symbolic_analytics_report_t& report,
    std::string_view source_label,
    const tsiemene::component_report_identity_t& report_identity,
    std::string_view contract_hash) {
  auto sequence_report = make_sequence_symbolic_analytics_report(report);
  sequence_report.schema = report.schema;
  auto document = make_symbolic_runtime_lls_document_(
      sequence_report,
      source_symbolic_report_keys_(),
      source_label,
      report_identity);
  const std::string_view normalized_contract_hash =
      trim_ascii_ws_view_(contract_hash);
  if (!normalized_contract_hash.empty()) {
    document.entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "contract_hash", std::string(normalized_contract_hash)));
  }
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      document);
}

std::string sequence_symbolic_analytics_to_pretty_text(
    const sequence_symbolic_analytics_report_t& report,
    std::string_view sequence_label,
    std::string_view output_filename,
    bool use_color,
    sequence_symbolic_report_compaction_options_t compaction_options) {
  const auto compacted =
      compact_sequence_symbolic_analytics_report(report, compaction_options);
  return symbolic_analytics_to_pretty_text_(
      compacted,
      generic_symbolic_pretty_config_(),
      sequence_label,
      output_filename,
      use_color);
}

std::string data_symbolic_analytics_to_pretty_text(
    const data_symbolic_analytics_report_t& report,
    std::string_view source_label,
    std::string_view output_filename,
    bool use_color) {
  auto sequence_report = make_sequence_symbolic_analytics_report(report);
  sequence_report.schema = report.schema;
  return symbolic_analytics_to_pretty_text_(
      sequence_report,
      source_symbolic_pretty_config_(),
      source_label,
      output_filename,
      use_color);
}

bool write_sequence_analytics_file(
    const sequence_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view sequence_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = sequence_analytics_to_latent_lineage_state_text(
        report, options, sequence_label, report_identity);
  } catch (const std::exception& e) {
    if (error) {
      *error =
          "cannot serialize sequence analytics report: " + std::string(e.what());
    }
    return false;
  }
  return write_text_file_atomically_(
      payload, output_file, "sequence analytics", error);
}

bool write_data_analytics_file(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view source_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity,
    std::string_view contract_hash) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = data_analytics_to_latent_lineage_state_text(
        report, options, source_label, report_identity, contract_hash);
  } catch (const std::exception& e) {
    if (error) *error = "cannot serialize data analytics report: " + std::string(e.what());
    return false;
  }
  return write_text_file_atomically_(payload, output_file, "data analytics", error);
}

bool write_sequence_symbolic_analytics_file(
    const sequence_symbolic_analytics_report_t& report,
    const std::filesystem::path& output_file,
    std::string_view sequence_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity,
    sequence_symbolic_report_compaction_options_t compaction_options) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = sequence_symbolic_analytics_to_latent_lineage_state_text(
        report, sequence_label, report_identity, compaction_options);
  } catch (const std::exception& e) {
    if (error) {
      *error =
          "cannot serialize sequence symbolic analytics report: " +
          std::string(e.what());
    }
    return false;
  }
  return write_text_file_atomically_(
      payload, output_file, "sequence symbolic analytics", error);
}

bool write_data_symbolic_analytics_file(
    const data_symbolic_analytics_report_t& report,
    const std::filesystem::path& output_file,
    std::string_view source_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity,
    std::string_view contract_hash) {
  if (error) error->clear();

  std::string payload;
  try {
    payload = data_symbolic_analytics_to_latent_lineage_state_text(
        report, source_label, report_identity, contract_hash);
  } catch (const std::exception& e) {
    if (error) {
      *error =
          "cannot serialize symbolic data analytics report: " +
          std::string(e.what());
    }
    return false;
  }
  return write_text_file_atomically_(
      payload, output_file, "symbolic data analytics", error);
}

std::string extract_data_analytics_kv_schema(std::string_view payload) {
  runtime_lls_document_t document{};
  std::string parse_error;
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &document, &parse_error)) {
    return {};
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, &parse_error)) {
    return {};
  }
  if (const auto it = kv.find("schema"); it != kv.end()) return it->second;
  return {};
}

std::string extract_sequence_analytics_kv_schema(std::string_view payload) {
  return extract_data_analytics_kv_schema(payload);
}

std::string extract_sequence_symbolic_analytics_kv_schema(
    std::string_view payload) {
  return extract_data_analytics_kv_schema(payload);
}

std::string extract_data_symbolic_analytics_kv_schema(std::string_view payload) {
  return extract_data_analytics_kv_schema(payload);
}

bool is_supported_sequence_analytics_schema(std::string_view schema) {
  return schema == kSequenceAnalyticsSchemaCurrent ||
         schema == kEmbeddingSequenceAnalyticsSchemaCurrent;
}

bool is_supported_data_analytics_schema(std::string_view schema) {
  return schema == kDataAnalyticsSchemaCurrent;
}

bool is_supported_sequence_symbolic_analytics_schema(std::string_view schema) {
  return schema == kSequenceAnalyticsSymbolicSchemaCurrent ||
         schema == kEmbeddingSequenceAnalyticsSymbolicSchemaCurrent;
}

bool is_supported_data_symbolic_analytics_schema(std::string_view schema) {
  return schema == kDataAnalyticsSymbolicSchemaCurrent;
}

std::filesystem::path source_data_analytics_root_directory() {
  return cuwacunu::hashimyei::canonical_path_directory(
      cuwacunu::hashimyei::store_root(), "tsi.source.dataloader");
}

std::filesystem::path source_data_analytics_contract_directory(
    std::string_view contract_hash) {
  const std::string token = contract_hash_path_token_(contract_hash);
  if (token.empty()) return {};
  return source_data_analytics_root_directory() / "contracts" / token;
}

std::filesystem::path source_data_analytics_context_directory(
    std::string_view contract_hash,
    std::string_view canonical_path,
    std::string_view source_runtime_cursor) {
  const std::string contract_token = contract_hash_path_token_(contract_hash);
  if (contract_token.empty()) return {};
  const std::string canonical_path_token = analytics_path_token_(canonical_path);
  if (canonical_path_token.empty()) return {};
  const std::string source_runtime_token =
      analytics_path_token_(source_runtime_cursor);
  return cuwacunu::hashimyei::canonical_path_directory(
             cuwacunu::hashimyei::store_root(), canonical_path) /
         "contracts" / contract_token / "contexts" /
         (source_runtime_token.empty() ? std::string("default")
                                       : source_runtime_token);
}

std::filesystem::path source_data_analytics_latest_file_path(
    std::string_view contract_hash,
    std::string_view canonical_path,
    std::string_view source_runtime_cursor) {
  const auto base = source_data_analytics_context_directory(
      contract_hash, canonical_path, source_runtime_cursor);
  if (base.empty()) return {};
  return base / std::string(kDataAnalyticsLatestReportFilename);
}

std::filesystem::path source_data_analytics_symbolic_latest_file_path(
    std::string_view contract_hash,
    std::string_view canonical_path,
    std::string_view source_runtime_cursor) {
  const auto base = source_data_analytics_context_directory(
      contract_hash, canonical_path, source_runtime_cursor);
  if (base.empty()) return {};
  return base / std::string(kDataAnalyticsSymbolicLatestReportFilename);
}

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
