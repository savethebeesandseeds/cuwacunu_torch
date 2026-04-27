std::string extract_analytics_kv_schema(std::string_view payload) {
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

bool is_supported_network_analytics_schema(std::string_view schema) {
  return schema == kNetworkAnalyticsSchemaCurrent;
}

bool is_supported_network_design_analytics_schema(std::string_view schema) {
  return schema == kNetworkDesignAnalyticsSchemaCurrent;
}

std::string network_analytics_to_latent_lineage_state_text(
    const network_analytics_report_t& report,
    std::string_view checkpoint_filename,
    const tsiemene::component_report_identity_t& report_identity) {
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      make_runtime_lls_document_(report, checkpoint_filename, report_identity));
}

std::string network_analytics_to_pretty_text(
    const network_analytics_report_t& report,
    std::string_view network_label,
    bool use_color) {
  return as_pretty_text_(report, network_label, use_color);
}

std::string network_design_analytics_to_latent_lineage_state_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label) {
  return cuwacunu::camahjucunu::dsl::convert_latent_lineage_state_payload_to_lattice_state(
      as_ascii_key_value_(report, source_label));
}

std::string network_design_analytics_to_pretty_text(
    const network_design_analytics_report_t& report,
    std::string_view source_label,
    bool use_color) {
  return as_pretty_text_(report, source_label, use_color);
}

bool write_network_analytics_file(
    const torch::nn::Module& model,
    const std::filesystem::path& output_file,
    const network_analytics_options_t& options,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();
  const network_analytics_options_t effective = normalize_options_(options);
  network_analytics_report_t report{};
  try {
    report = summarize_module_network_analytics(model, effective);
  } catch (const std::exception& e) {
    if (error) {
      *error = "network analytics summarization failed: " + std::string(e.what());
    }
    return false;
  } catch (...) {
    if (error) *error = "network analytics summarization failed: unknown error";
    return false;
  }

  std::error_code ec;
  auto parent = output_file.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) *error = "cannot create report directory: " + parent.string();
      return false;
    }
  }

  std::string payload;
  try {
    payload = network_analytics_to_latent_lineage_state_text(
        report, {}, report_identity);
  } catch (const std::exception& e) {
    if (error) {
      *error = "cannot serialize network analytics report: " + std::string(e.what());
    }
    return false;
  }
  return write_text_file_atomically_(
      payload, output_file, "network analytics report", error);
}

bool write_network_analytics_sidecar_for_checkpoint(
    const torch::nn::Module& model,
    const std::filesystem::path& checkpoint_file,
    std::filesystem::path* out_sidecar_file,
    const network_analytics_options_t& options,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();
  const network_analytics_options_t effective = normalize_options_(options);
  std::filesystem::path sidecar = checkpoint_file;
  if (sidecar.extension() == ".pt") {
    sidecar.replace_extension(".network_analytics.lls");
  } else {
    sidecar += ".network_analytics.lls";
  }

  network_analytics_report_t report{};
  try {
    report = summarize_module_network_analytics(model, effective);
  } catch (const std::exception& e) {
    if (error) {
      *error = "network analytics summarization failed: " + std::string(e.what());
    }
    return false;
  } catch (...) {
    if (error) *error = "network analytics summarization failed: unknown error";
    return false;
  }

  std::error_code ec;
  auto parent = sidecar.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) *error = "cannot create report directory: " + parent.string();
      return false;
    }
  }

  std::string payload;
  try {
    payload = network_analytics_to_latent_lineage_state_text(
        report, checkpoint_file.filename().string(), report_identity);
  } catch (const std::exception& e) {
    if (error) {
      *error = "cannot serialize network analytics report: " + std::string(e.what());
    }
    return false;
  }
  if (!write_text_file_atomically_(
          payload, sidecar, "network analytics sidecar", error)) {
    return false;
  }

  if (out_sidecar_file) *out_sidecar_file = sidecar;
  return true;
}

} /* namespace torch_compat */
} /* namespace piaabo */
} /* namespace cuwacunu */
