[[nodiscard]] inline bool
normalize_wikimyei_hex_hash(std::string_view hashimyei,
                            std::string *out_hashimyei,
                            std::uint64_t *out_ordinal = nullptr) {
  return cuwacunu::hashimyei::normalize_hex_hash_name(hashimyei, out_hashimyei,
                                                      out_ordinal);
}

[[nodiscard]] inline bool parse_wikimyei_hex_hash(std::string_view s,
                                                  std::uint64_t *out) {
  return normalize_wikimyei_hex_hash(s, nullptr, out);
}

[[nodiscard]] inline std::string format_wikimyei_hex_hash(std::uint64_t value) {
  std::ostringstream oss;
  oss << "0x" << std::hex << std::nouppercase << std::setfill('0')
      << std::setw(4) << value;
  return oss.str();
}

[[nodiscard]] inline std::filesystem::path
wikimyei_representation_encoding_vicreg_store_root() {
  return cuwacunu::hashimyei::canonical_path_directory(
      cuwacunu::hashimyei::store_root(),
      "tsi.wikimyei.representation.encoding.vicreg");
}

[[nodiscard]] inline bool is_valid_wikimyei_representation_encoding_vicreg_hash(
    std::string_view hashimyei) {
  std::uint64_t parsed = 0;
  return normalize_wikimyei_hex_hash(hashimyei, nullptr, &parsed);
}

[[nodiscard]] inline bool
resolve_wikimyei_representation_encoding_vicreg_directory(
    std::string_view hashimyei, bool require_existing,
    std::filesystem::path *out_directory, std::string *error = nullptr) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();
  if (!out_directory) {
    if (error)
      *error = "wikimyei report_fragment directory output pointer is null";
    return false;
  }

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_hex_hash(hashimyei, &normalized_hashimyei)) {
    if (error)
      *error = "invalid wikimyei hashimyei id: " + std::string(hashimyei);
    return false;
  }

  const fs::path store_root =
      wikimyei_representation_encoding_vicreg_store_root();
  const fs::path canonical_directory = store_root / normalized_hashimyei;

  std::error_code ec;
  if (fs::exists(canonical_directory, ec)) {
    if (!fs::is_directory(canonical_directory, ec)) {
      if (error) {
        *error = "wikimyei report_fragment path is not a directory: " +
                 canonical_directory.string();
      }
      return false;
    }
    *out_directory = canonical_directory;
    return true;
  }
  if (ec) {
    if (error) {
      *error = "cannot inspect wikimyei report_fragment directory: " +
               canonical_directory.string();
    }
    return false;
  }

  if (fs::exists(store_root, ec) && fs::is_directory(store_root, ec)) {
    fs::path matched_directory{};
    bool found_match = false;
    for (const auto &entry : fs::directory_iterator(store_root, ec)) {
      if (ec)
        break;
      if (!entry.is_directory())
        continue;

      std::string candidate_hashimyei{};
      if (!normalize_wikimyei_hex_hash(entry.path().filename().string(),
                                       &candidate_hashimyei)) {
        continue;
      }
      if (candidate_hashimyei != normalized_hashimyei)
        continue;

      if (found_match && entry.path() != matched_directory) {
        if (error) {
          *error = "ambiguous wikimyei report_fragment directories for "
                   "canonical id: " +
                   normalized_hashimyei;
        }
        return false;
      }
      matched_directory = entry.path();
      found_match = true;
    }
    if (ec) {
      if (error) {
        *error =
            "cannot scan wikimyei report_fragment root: " + store_root.string();
      }
      return false;
    }
    if (found_match) {
      *out_directory = matched_directory;
      return true;
    }
  } else if (ec) {
    if (error) {
      *error = "cannot inspect wikimyei report_fragment root: " +
               store_root.string();
    }
    return false;
  }

  if (require_existing) {
    if (error) {
      *error =
          "wikimyei report_fragment not found: " + canonical_directory.string();
    }
    return false;
  }

  *out_directory = canonical_directory;
  return true;
}

[[nodiscard]] inline std::vector<
    cuwacunu::hashimyei::report_fragment_identity_t>
list_wikimyei_representation_encoding_vicreg_report_fragments() {
  return cuwacunu::hashimyei::discover_created_report_fragments_for(
      "representation", "vicreg");
}

[[nodiscard]] inline std::vector<
    wikimyei_representation_encoding_vicreg_init_entry_t>
list_wikimyei_representation_encoding_vicreg_init_entries() {
  const auto report_fragments =
      list_wikimyei_representation_encoding_vicreg_report_fragments();
  std::vector<wikimyei_representation_encoding_vicreg_init_entry_t> out;
  out.reserve(report_fragments.size());
  for (const auto &item : report_fragments) {
    wikimyei_representation_encoding_vicreg_init_entry_t e{};
    e.hashimyei = item.hashimyei;
    e.canonical_path = item.canonical_path;
    e.report_fragment_directory = item.directory;
    e.weights_count = item.weight_files.size();
    out.push_back(std::move(e));
  }
  return out;
}

[[nodiscard]] inline bool
write_wikimyei_text_file(const std::filesystem::path &path,
                         const std::string &text, std::string *error) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error)
      *error = "cannot open file for write: " + path.string();
    return false;
  }
  out.write(text.data(), static_cast<std::streamsize>(text.size()));
  if (!out) {
    if (error)
      *error = "cannot write file contents: " + path.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline bool ensure_wikimyei_vicreg_weights_placeholder(
    const std::filesystem::path &weights_file, std::string *error) {
  namespace fs = std::filesystem;
  std::error_code ec;
  if (fs::exists(weights_file, ec) && fs::is_regular_file(weights_file, ec))
    return true;
  const std::string payload =
      "schema=tsi.wikimyei.representation.encoding.vicreg.weights.init.v1\n"
      "state=placeholder\n";
  return write_wikimyei_text_file(weights_file, payload, error);
}

[[nodiscard]] inline std::string
next_wikimyei_representation_encoding_vicreg_hash(
    const std::filesystem::path &report_fragments_root) {
  namespace fs = std::filesystem;
  std::error_code ec;
  std::uint64_t max_seen = 0;
  bool seen_any = false;

  if (fs::exists(report_fragments_root, ec) &&
      fs::is_directory(report_fragments_root, ec)) {
    for (const auto &entry :
         fs::directory_iterator(report_fragments_root, ec)) {
      if (ec)
        break;
      if (!entry.is_directory())
        continue;

      const std::string hash = entry.path().filename().string();
      std::uint64_t parsed = 0;
      if (!parse_wikimyei_hex_hash(hash, &parsed))
        continue;
      if (!seen_any || parsed > max_seen)
        max_seen = parsed;
      seen_any = true;
    }
  }

  if (!seen_any)
    return "0x0000";
  return format_wikimyei_hex_hash(max_seen + 1);
}

inline constexpr std::string_view kWikimyeiEncodingVicregCanonicalType =
    "tsi.wikimyei.representation.encoding.vicreg";
inline constexpr std::string_view kWikimyeiEncodingVicregFamily =
    "representation";
inline constexpr std::string_view kWikimyeiEncodingVicregModel = "vicreg";

[[nodiscard]] inline bool
save_wikimyei_representation_encoding_vicreg_report_fragment_with_driver(
    const cuwacunu::hashimyei::report_fragment_action_context_t &action,
    std::string *error) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_hex_hash(action.report_fragment_id,
                                   &normalized_hashimyei)) {
    if (error)
      *error = "invalid wikimyei hashimyei id: " + action.report_fragment_id;
    return false;
  }

  fs::path report_fragment_directory{};
  if (!resolve_wikimyei_representation_encoding_vicreg_directory(
          normalized_hashimyei, false, &report_fragment_directory, error)) {
    return false;
  }

  std::error_code ec;
  fs::create_directories(report_fragment_directory, ec);
  if (ec) {
    if (error)
      *error = "cannot create wikimyei report_fragment directory: " +
               report_fragment_directory.string();
    return false;
  }

  const fs::path weights_file = report_fragment_directory / "weights.init.pt";
  fs::path weights_network_analytics_file = weights_file;
  weights_network_analytics_file.replace_extension(".network_analytics.lls");
  bool wrote_weights_network_analytics_file = false;
  fs::path embedding_sequence_analytics_file =
      report_fragment_directory /
      std::string(cuwacunu::piaabo::torch_compat::
                      kEmbeddingSequenceAnalyticsLatestReportFilename);
  bool wrote_embedding_sequence_analytics_file = false;
  fs::path embedding_sequence_symbolic_analytics_file =
      report_fragment_directory /
      std::string(cuwacunu::piaabo::torch_compat::
                      kEmbeddingSequenceAnalyticsSymbolicLatestReportFilename);
  bool wrote_embedding_sequence_symbolic_analytics_file = false;
  const fs::path train_summary_lls_file =
      report_fragment_directory / "train_summary.latest.lls";
  bool wrote_train_summary_lls_file = false;
  const fs::path status_lls_file =
      report_fragment_directory / "status.latest.lls";
  bool wrote_status_lls_file = false;
  auto *out =
      static_cast<wikimyei_representation_encoding_vicreg_init_record_t *>(
          action.user_data);
  const bool enable_network_analytics_sidecar =
      !out || out->enable_network_analytics_sidecar;
  const bool enable_embedding_sequence_analytics_sidecar =
      !out || out->enable_embedding_sequence_analytics_sidecar;
  const std::string contract_hash = out ? out->contract_hash : std::string{};
  const std::string binding_id = out ? out->binding_id : std::string{};
  const std::string run_id = out ? out->run_id : std::string{};
  const bool has_wave_cursor = out && out->has_wave_cursor;
  const std::uint64_t wave_cursor = out ? out->wave_cursor : 0;
  const std::string canonical_path =
      std::string(kWikimyeiEncodingVicregCanonicalType) + "." +
      normalized_hashimyei;
  const std::string effective_run_id =
      effective_runtime_report_run_id_(run_id, normalized_hashimyei);
  std::vector<fs::path> history_files{};
  const auto write_history_copy = [&](const fs::path &stable_file) -> bool {
    std::error_code file_ec;
    if (!fs::exists(stable_file, file_ec) ||
        !fs::is_regular_file(stable_file, file_ec)) {
      return true;
    }
    const fs::path history_file =
        runtime_history_file_path_(stable_file, effective_run_id);
    if (history_file == stable_file)
      return true;
    fs::copy_file(stable_file, history_file,
                  fs::copy_options::overwrite_existing, file_ec);
    if (file_ec) {
      if (error) {
        *error = "cannot persist vicreg runtime history file: " +
                 history_file.string();
      }
      return false;
    }
    history_files.push_back(history_file);
    return true;
  };
  if (action.object_handle) {
    // Contract: object_handle points to
    // cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4.
    auto *model = static_cast<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *>(
        action.object_handle);
    try {
      model->save(weights_file.string());
    } catch (const std::exception &e) {
      if (error)
        *error = "vicreg save failed: " + std::string(e.what());
      return false;
    }

    if (enable_network_analytics_sidecar) {
      std::string report_error;
      if (write_vicreg_network_analytics_sidecar_(
              *model, weights_file, canonical_path,
              kWikimyeiEncodingVicregCanonicalType, normalized_hashimyei,
              contract_hash, binding_id, effective_run_id,
              out ? std::string_view(out->source_runtime_cursor)
                  : std::string_view{},
              has_wave_cursor, wave_cursor, &weights_network_analytics_file,
              &report_error)) {
        wrote_weights_network_analytics_file = true;
      } else {
        log_warn("[tsi.vicreg] network analytics report skipped for '%s': %s\n",
                 weights_file.string().c_str(), report_error.c_str());
      }
    }

    if (enable_embedding_sequence_analytics_sidecar && out &&
        out->has_embedding_sequence_analytics) {
      auto embedding_report = out->embedding_sequence_analytics_report;
      embedding_report.schema =
          std::string(cuwacunu::piaabo::torch_compat::
                          kEmbeddingSequenceAnalyticsSchemaCurrent);
      auto embedding_symbolic_report =
          out->embedding_sequence_symbolic_analytics_report;
      embedding_symbolic_report.schema =
          std::string(cuwacunu::piaabo::torch_compat::
                          kEmbeddingSequenceAnalyticsSymbolicSchemaCurrent);

      auto embedding_report_identity = make_component_report_identity(
          canonical_path, binding_id,
          cuwacunu::piaabo::latent_lineage_state::report_taxonomy::
              kEmbeddingData);
      embedding_report_identity.source_runtime_cursor =
          out->source_runtime_cursor;
      embedding_report_identity.has_wave_cursor = has_wave_cursor;
      embedding_report_identity.wave_cursor = wave_cursor;
      auto embedding_symbolic_report_identity = make_component_report_identity(
          canonical_path, binding_id,
          cuwacunu::piaabo::latent_lineage_state::report_taxonomy::
              kEmbeddingData);
      embedding_symbolic_report_identity.source_runtime_cursor =
          out->source_runtime_cursor;
      embedding_symbolic_report_identity.has_wave_cursor = has_wave_cursor;
      embedding_symbolic_report_identity.wave_cursor = wave_cursor;

      std::string write_error;
      if (cuwacunu::piaabo::torch_compat::write_sequence_analytics_file(
              embedding_report, out->embedding_sequence_analytics_options,
              embedding_sequence_analytics_file, canonical_path, &write_error,
              embedding_report_identity)) {
        wrote_embedding_sequence_analytics_file = true;
      } else {
        log_warn(
            "[tsi.vicreg] embedding analytics report skipped for '%s': %s\n",
            embedding_sequence_analytics_file.string().c_str(),
            write_error.c_str());
      }

      if (cuwacunu::piaabo::torch_compat::
              write_sequence_symbolic_analytics_file(
                  embedding_symbolic_report,
                  embedding_sequence_symbolic_analytics_file, canonical_path,
                  &write_error, embedding_symbolic_report_identity)) {
        wrote_embedding_sequence_symbolic_analytics_file = true;
      } else {
        log_warn("[tsi.vicreg] embedding symbolic analytics report skipped for "
                 "'%s': %s\n",
                 embedding_sequence_symbolic_analytics_file.string().c_str(),
                 write_error.c_str());
      }
    }

  } else {
    std::string io_error;
    if (!ensure_wikimyei_vicreg_weights_placeholder(weights_file, &io_error)) {
      if (error)
        *error = io_error;
      return false;
    }
  }
  if (!wrote_weights_network_analytics_file) {
    std::error_code stale_ec;
    (void)fs::remove(weights_network_analytics_file, stale_ec);
  }
  if (!wrote_embedding_sequence_analytics_file) {
    std::error_code stale_ec;
    (void)fs::remove(embedding_sequence_analytics_file, stale_ec);
  }
  if (!wrote_embedding_sequence_symbolic_analytics_file) {
    std::error_code stale_ec;
    (void)fs::remove(embedding_sequence_symbolic_analytics_file, stale_ec);
  }
  if (!write_history_copy(weights_file))
    return false;

  const std::string canonical_action =
      action.canonical_action.empty()
          ? "tsi.wikimyei.representation.encoding.vicreg.init()"
          : action.canonical_action;

  std::ostringstream metadata;
  metadata << "schema=tsi.wikimyei.representation.encoding.vicreg.init.v1\n";
  metadata << "canonical_action=" << canonical_action << "\n";
  metadata << "canonical_target=tsi.wikimyei.representation.encoding.vicreg\n";
  metadata << "family=" << kWikimyeiEncodingVicregFamily << "\n";
  metadata << "model=" << kWikimyeiEncodingVicregModel << "\n";
  metadata << "hashimyei=" << normalized_hashimyei << "\n";
  metadata << "canonical_path=" << canonical_path << "\n";
  if (out && (!out->component_tag.empty() ||
              !out->component_compatibility_sha256_hex.empty() ||
              !out->docking_signature_sha256_hex.empty())) {
    metadata << "component_tag=" << out->component_tag << "\n";
    metadata << "component_compatibility_sha256_hex="
             << out->component_compatibility_sha256_hex << "\n";
    metadata << "docking_signature_sha256_hex="
             << out->docking_signature_sha256_hex << "\n";
    for (const auto &[field, value] :
         cuwacunu::camahjucunu::instrument_signature_fields(
             out->instrument_signature)) {
      metadata << "instrument_signature." << field << "=" << value << "\n";
    }
    for (const auto &[field, value] :
         cuwacunu::camahjucunu::instrument_signature_fields(
             out->runtime_instrument_signature)) {
      metadata << "runtime_instrument_signature." << field << "=" << value
               << "\n";
    }
  }
  metadata << "weights_file=" << weights_file.filename().string() << "\n";
  metadata << "status_file=" << status_lls_file.filename().string() << "\n";
  if (wrote_weights_network_analytics_file) {
    metadata << "weights_network_analytics_file="
             << weights_network_analytics_file.filename().string() << "\n";
  }
  if (wrote_embedding_sequence_analytics_file) {
    metadata << "embedding_sequence_analytics_file="
             << embedding_sequence_analytics_file.filename().string() << "\n";
  }
  if (wrote_embedding_sequence_symbolic_analytics_file) {
    metadata << "embedding_sequence_symbolic_analytics_file="
             << embedding_sequence_symbolic_analytics_file.filename().string()
             << "\n";
  }
  bool metadata_encrypted = false;
  bool metadata_plaintext_fallback = false;
  std::string metadata_warning;

  std::string metadata_error;
  if (cuwacunu::hashimyei::write_encrypted_metadata(
          report_fragment_directory, metadata.str(), &metadata_error)) {
    metadata_encrypted = true;
    std::error_code stale_ec;
    (void)fs::remove(report_fragment_directory / "metadata.txt", stale_ec);
  } else {
    metadata_warning = metadata_error;
    std::string io_error;
    if (!write_wikimyei_text_file(report_fragment_directory / "metadata.txt",
                                  metadata.str(), &io_error)) {
      if (error) {
        *error =
            "cannot persist metadata (encrypted failed: " + metadata_error +
            "; plaintext failed: " + io_error + ")";
      }
      return false;
    }
    metadata_plaintext_fallback = true;
    std::error_code stale_ec;
    (void)fs::remove(report_fragment_directory / "metadata.enc", stale_ec);
  }
  if (!write_history_copy(report_fragment_directory / "metadata.enc")) {
    return false;
  }
  if (!write_history_copy(report_fragment_directory / "metadata.txt")) {
    return false;
  }

  {
    const auto now_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    std::uint64_t trained_steps = 0;
    std::uint64_t trained_epochs = 0;
    std::uint64_t trained_samples = 0;
    std::uint64_t skipped_batches = 0;
    int accumulate_steps = 1;
    bool has_pending_grad = false;
    double clip_norm = 0.0;
    double clip_value = 0.0;
    double last_committed_loss_mean = 0.0;
    double last_committed_inv_mean = 0.0;
    double last_committed_var_mean = 0.0;
    double last_committed_cov_raw_mean = 0.0;
    double last_committed_cov_weighted_mean = 0.0;
    if (action.object_handle) {
      const auto *model =
          static_cast<const cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *>(
              action.object_handle);
      trained_steps = static_cast<std::uint64_t>(
          std::max(0, model->runtime_state.optimizer_steps));
      trained_epochs = model->runtime_state.trained_epochs;
      trained_samples = model->runtime_state.trained_samples;
      skipped_batches = model->runtime_state.skipped_batches;
      accumulate_steps = std::max(1, model->training_policy.accumulate_steps);
      has_pending_grad = model->runtime_state.has_pending_grad;
      clip_norm = model->training_policy.clip_norm;
      clip_value = model->training_policy.clip_value;
      last_committed_loss_mean = model->runtime_state.last_committed_loss_mean;
      last_committed_inv_mean = model->runtime_state.last_committed_inv_mean;
      last_committed_var_mean = model->runtime_state.last_committed_var_mean;
      last_committed_cov_raw_mean =
          model->runtime_state.last_committed_cov_raw_mean;
      last_committed_cov_weighted_mean =
          model->runtime_state.last_committed_cov_weighted_mean;
    }
    if (out && out->has_train_summary) {
      runtime_lls_document_t train_summary_lls{};
      append_runtime_string_entry_(
          &train_summary_lls, "schema",
          "tsi.wikimyei.representation.encoding.vicreg.train_summary.v1");
      {
        auto train_summary_identity = make_component_report_identity(
            canonical_path, binding_id,
            cuwacunu::piaabo::latent_lineage_state::report_taxonomy::
                kEmbeddingNetwork);
        train_summary_identity.source_runtime_cursor =
            out->source_runtime_cursor;
        train_summary_identity.has_wave_cursor = has_wave_cursor;
        train_summary_identity.wave_cursor = wave_cursor;
        cuwacunu::piaabo::latent_lineage_state::
            append_runtime_report_header_entries(
                &train_summary_lls,
                make_runtime_report_header(train_summary_identity));
      }
      append_runtime_string_entry_(
          &train_summary_lls, "report_event",
          report_event_token(report_event_e::EpochEnd));
      append_runtime_string_entry_if_nonempty_(&train_summary_lls,
                                               "contract_hash", contract_hash);
      append_runtime_component_evidence_entries_(&train_summary_lls, out);
      if (action.object_handle) {
        const auto *model =
            static_cast<const cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *>(
                action.object_handle);
        append_runtime_string_entry_if_nonempty_(&train_summary_lls,
                                                 "component_instance_name",
                                                 model->component_name);
      }
      append_runtime_u64_entry_(&train_summary_lls, "train.accumulate_steps",
                                static_cast<std::uint64_t>(accumulate_steps));
      append_runtime_u64_entry_(&train_summary_lls, "train.epoch_index",
                                out->train_summary_snapshot.epoch_index);
      append_runtime_u64_entry_(&train_summary_lls, "train.optimizer_steps",
                                out->train_summary_snapshot.optimizer_steps);
      append_runtime_u64_entry_(&train_summary_lls, "train.skipped_batches",
                                out->train_summary_snapshot.skipped_batches);
      append_runtime_u64_entry_(&train_summary_lls, "train.trained_epochs",
                                out->train_summary_snapshot.trained_epochs);
      append_runtime_u64_entry_(&train_summary_lls, "train.trained_samples",
                                out->train_summary_snapshot.trained_samples);
      append_runtime_double_entry_(&train_summary_lls, "train.loss.epoch_mean",
                                   out->train_summary_snapshot.epoch_loss_mean);
      append_runtime_double_entry_(&train_summary_lls, "train.loss.inv_mean",
                                   out->train_summary_snapshot.epoch_inv_mean);
      append_runtime_double_entry_(&train_summary_lls, "train.loss.var_mean",
                                   out->train_summary_snapshot.epoch_var_mean);
      append_runtime_double_entry_(
          &train_summary_lls, "train.loss.cov_raw_mean",
          out->train_summary_snapshot.epoch_cov_raw_mean);
      append_runtime_double_entry_(
          &train_summary_lls, "train.loss.cov_weighted_mean",
          out->train_summary_snapshot.epoch_cov_weighted_mean);
      append_runtime_double_entry_(
          &train_summary_lls, "train.loss.last_committed_mean",
          out->train_summary_snapshot.last_committed_loss_mean);
      append_runtime_double_entry_(&train_summary_lls, "train.lr.current",
                                   out->train_summary_snapshot.learning_rate);
      std::string train_summary_error;
      std::string train_summary_payload;
      try {
        train_summary_payload =
            cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
                train_summary_lls);
      } catch (const std::exception &e) {
        if (error) {
          *error =
              "cannot serialize train summary report: " + std::string(e.what());
        }
        return false;
      }
      if (!write_wikimyei_text_file(train_summary_lls_file,
                                    train_summary_payload,
                                    &train_summary_error)) {
        if (error) {
          *error =
              "cannot persist train summary report: " + train_summary_error;
        }
        return false;
      }
      wrote_train_summary_lls_file = true;
    }
    runtime_lls_document_t status_lls{};
    append_runtime_string_entry_(
        &status_lls, "schema",
        "tsi.wikimyei.representation.encoding.vicreg.status.v1");
    {
      auto status_identity =
          make_component_report_identity(canonical_path, binding_id);
      status_identity.source_runtime_cursor =
          out ? out->source_runtime_cursor : std::string{};
      status_identity.has_wave_cursor = has_wave_cursor;
      status_identity.wave_cursor = wave_cursor;
      cuwacunu::piaabo::latent_lineage_state::
          append_runtime_report_header_entries(
              &status_lls, make_runtime_report_header(status_identity));
    }
    append_runtime_u64_entry_(&status_lls, "trained_epochs", trained_epochs);
    append_runtime_u64_entry_(&status_lls, "trained_steps", trained_steps);
    append_runtime_u64_entry_(&status_lls, "trained_samples", trained_samples);
    append_runtime_string_entry_if_nonempty_(&status_lls, "contract_hash",
                                             contract_hash);
    append_runtime_component_evidence_entries_(&status_lls, out);
    append_runtime_u64_entry_(&status_lls, "skipped_batches", skipped_batches);
    append_runtime_u64_entry_(&status_lls, "accumulate_steps",
                              static_cast<std::uint64_t>(accumulate_steps));
    append_runtime_bool_entry_(&status_lls, "has_pending_grad",
                               has_pending_grad);
    append_runtime_double_entry_(&status_lls, "clip_norm", clip_norm);
    append_runtime_double_entry_(&status_lls, "clip_value", clip_value);
    append_runtime_double_entry_(&status_lls, "last_committed_loss_mean",
                                 last_committed_loss_mean);
    append_runtime_double_entry_(&status_lls, "last_committed_inv_mean",
                                 last_committed_inv_mean);
    append_runtime_double_entry_(&status_lls, "last_committed_var_mean",
                                 last_committed_var_mean);
    append_runtime_double_entry_(&status_lls, "last_committed_cov_raw_mean",
                                 last_committed_cov_raw_mean);
    append_runtime_double_entry_(&status_lls,
                                 "last_committed_cov_weighted_mean",
                                 last_committed_cov_weighted_mean);
    append_runtime_string_entry_(&status_lls, "last_trial_id",
                                 effective_run_id);
    append_runtime_string_entry_(&status_lls, "last_wave_hash", "");
    append_runtime_string_entry_(&status_lls, "last_contract_hash",
                                 contract_hash);
    append_runtime_u64_entry_(&status_lls, "updated_at_ms", now_ms);
    std::string status_error;
    std::string status_payload;
    try {
      status_payload =
          cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
              status_lls);
    } catch (const std::exception &e) {
      if (error) {
        *error = "cannot serialize status report: " + std::string(e.what());
      }
      return false;
    }
    if (!write_wikimyei_text_file(status_lls_file, status_payload,
                                  &status_error)) {
      if (error)
        *error = "cannot persist status report: " + status_error;
      return false;
    }
    wrote_status_lls_file = true;
  }
  if (wrote_train_summary_lls_file &&
      !write_history_copy(train_summary_lls_file)) {
    return false;
  }
  if (!write_history_copy(status_lls_file))
    return false;
  if (wrote_weights_network_analytics_file &&
      !write_history_copy(weights_network_analytics_file)) {
    return false;
  }
  if (wrote_embedding_sequence_analytics_file &&
      !write_history_copy(embedding_sequence_analytics_file)) {
    return false;
  }
  if (wrote_embedding_sequence_symbolic_analytics_file &&
      !write_history_copy(embedding_sequence_symbolic_analytics_file)) {
    return false;
  }

  cuwacunu::hashimyei::report_fragment_manifest_t manifest{};
  manifest.family_canonical_path =
      std::string(kWikimyeiEncodingVicregCanonicalType);
  manifest.family = std::string(kWikimyeiEncodingVicregFamily);
  manifest.model = std::string(kWikimyeiEncodingVicregModel);
  manifest.report_fragment_id = normalized_hashimyei;
  {
    std::error_code size_ec;
    const auto weights_size = fs::file_size(weights_file, size_ec);
    manifest.files.push_back(
        cuwacunu::hashimyei::report_fragment_manifest_file_t{
            .path = weights_file.filename().string(),
            .size = size_ec ? 0 : weights_size,
        });
  }
  const auto append_manifest_file_if_present = [&](const fs::path &path) {
    std::error_code file_ec;
    if (!fs::exists(path, file_ec) || !fs::is_regular_file(path, file_ec))
      return;
    const auto size = fs::file_size(path, file_ec);
    manifest.files.push_back(
        cuwacunu::hashimyei::report_fragment_manifest_file_t{
            .path = path.filename().string(),
            .size = file_ec ? 0 : size,
        });
  };
  append_manifest_file_if_present(report_fragment_directory / "metadata.enc");
  append_manifest_file_if_present(report_fragment_directory / "metadata.txt");
  if (wrote_train_summary_lls_file) {
    append_manifest_file_if_present(train_summary_lls_file);
  }
  if (wrote_status_lls_file) {
    append_manifest_file_if_present(status_lls_file);
  }
  if (wrote_weights_network_analytics_file) {
    append_manifest_file_if_present(weights_network_analytics_file);
  }
  if (wrote_embedding_sequence_analytics_file) {
    append_manifest_file_if_present(embedding_sequence_analytics_file);
  }
  if (wrote_embedding_sequence_symbolic_analytics_file) {
    append_manifest_file_if_present(embedding_sequence_symbolic_analytics_file);
  }
  for (const auto &history_file : history_files) {
    append_manifest_file_if_present(history_file);
  }
  std::string manifest_error;
  if (!cuwacunu::hashimyei::write_report_fragment_manifest(
          report_fragment_directory, manifest, &manifest_error)) {
    if (error)
      *error = "cannot persist report_fragment manifest: " + manifest_error;
    return false;
  }

  if (out) {
    out->hashimyei = normalized_hashimyei;
    out->canonical_path = canonical_path;
    out->report_fragment_directory = report_fragment_directory;
    out->weights_file = weights_file;
    out->embedding_sequence_analytics_file =
        wrote_embedding_sequence_analytics_file
            ? embedding_sequence_analytics_file
            : fs::path{};
    out->embedding_sequence_symbolic_analytics_file =
        wrote_embedding_sequence_symbolic_analytics_file
            ? embedding_sequence_symbolic_analytics_file
            : fs::path{};
    out->train_summary_file =
        wrote_train_summary_lls_file ? train_summary_lls_file : fs::path{};
    out->metadata_encrypted = metadata_encrypted;
    out->metadata_plaintext_fallback = metadata_plaintext_fallback;
    out->metadata_warning = metadata_warning;
  }
  return true;
}

[[nodiscard]] inline bool
load_wikimyei_representation_encoding_vicreg_report_fragment_with_driver(
    const cuwacunu::hashimyei::report_fragment_action_context_t &action,
    std::string *error) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_hex_hash(action.report_fragment_id,
                                   &normalized_hashimyei)) {
    if (error)
      *error = "invalid wikimyei hashimyei id: " + action.report_fragment_id;
    return false;
  }

  fs::path report_fragment_directory{};
  if (!resolve_wikimyei_representation_encoding_vicreg_directory(
          normalized_hashimyei, true, &report_fragment_directory, error)) {
    return false;
  }

  const fs::path weights_file = report_fragment_directory / "weights.init.pt";
  std::error_code ec;
  if (!fs::exists(weights_file, ec) || !fs::is_regular_file(weights_file, ec)) {
    if (error)
      *error = "vicreg report_fragment weights file not found: " +
               weights_file.string();
    return false;
  }

  if (cuwacunu::hashimyei::report_fragment_manifest_exists(
          report_fragment_directory)) {
    cuwacunu::hashimyei::report_fragment_manifest_t manifest{};
    std::string manifest_error;
    if (!cuwacunu::hashimyei::read_report_fragment_manifest(
            report_fragment_directory, &manifest, &manifest_error)) {
      if (error)
        *error = "cannot read report_fragment manifest: " + manifest_error;
      return false;
    }
    if (manifest.family_canonical_path !=
        std::string(kWikimyeiEncodingVicregCanonicalType)) {
      if (error) {
        *error = "report_fragment manifest family_canonical_path mismatch: " +
                 manifest.family_canonical_path;
      }
      return false;
    }
    if (!action.family.empty() && manifest.family != action.family) {
      if (error)
        *error = "report_fragment manifest family mismatch: " + manifest.family;
      return false;
    }
    if (!action.model.empty() && manifest.model != action.model) {
      if (error)
        *error = "report_fragment manifest model mismatch: " + manifest.model;
      return false;
    }
    if (manifest.report_fragment_id != normalized_hashimyei) {
      if (error)
        *error = "report_fragment manifest hashimyei mismatch: " +
                 manifest.report_fragment_id;
      return false;
    }
    if (!cuwacunu::hashimyei::report_fragment_manifest_has_file(
            manifest, weights_file.filename().string())) {
      if (error)
        *error = "report_fragment manifest missing weights file entry: " +
                 weights_file.filename().string();
      return false;
    }
  }

  if (action.object_handle) {
    // Contract: object_handle points to
    // cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4.
    auto *model = static_cast<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *>(
        action.object_handle);
    try {
      model->load(weights_file.string());
    } catch (const std::exception &e) {
      const std::string load_error = std::string(e.what());
      const bool cuda_runtime_only_failure =
          load_error.find("cudaGetDeviceCount") != std::string::npos ||
          load_error.find("cuda unavailable") != std::string::npos ||
          load_error.find("operation not supported on this OS") !=
              std::string::npos;
      if (cuda_runtime_only_failure && !model->device.is_cuda()) {
        log_warn("[tsi.vicreg] report_fragment load skipped (cuda runtime "
                 "unavailable). "
                 "Using fresh-initialized model for this run.\n");
        if (error)
          error->clear();
        return true;
      }
      if (error)
        *error = "vicreg load failed: " + load_error;
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool
ensure_wikimyei_representation_encoding_vicreg_driver_registered(
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (cuwacunu::hashimyei::has_report_fragment_driver(
          kWikimyeiEncodingVicregCanonicalType))
    return true;

  cuwacunu::hashimyei::report_fragment_driver_t driver{};
  driver.family_canonical_path =
      std::string(kWikimyeiEncodingVicregCanonicalType);
  driver.family = std::string(kWikimyeiEncodingVicregFamily);
  driver.model = std::string(kWikimyeiEncodingVicregModel);
  driver.save =
      save_wikimyei_representation_encoding_vicreg_report_fragment_with_driver;
  driver.load =
      load_wikimyei_representation_encoding_vicreg_report_fragment_with_driver;

  if (cuwacunu::hashimyei::register_report_fragment_driver(std::move(driver),
                                                           error))
    return true;
  // Registration may race in multi-entry contexts; treat "already registered"
  // as success.
  return cuwacunu::hashimyei::has_report_fragment_driver(
      kWikimyeiEncodingVicregCanonicalType);
}

[[nodiscard]] inline bool
write_wikimyei_representation_encoding_vicreg_report_fragment_payload(
    std::string canonical_action,
    wikimyei_representation_encoding_vicreg_init_record_t *out,
    cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *model = nullptr) {
  if (!out)
    return false;

  out->metadata_encrypted = false;
  out->metadata_plaintext_fallback = false;
  out->metadata_warning.clear();
  out->canonical_path.clear();
  out->weights_file.clear();
  out->train_summary_file.clear();

  std::string registration_error;
  if (!ensure_wikimyei_representation_encoding_vicreg_driver_registered(
          &registration_error)) {
    out->error = "failed to register vicreg report_fragment driver: " +
                 registration_error;
    return false;
  }

  cuwacunu::hashimyei::report_fragment_action_context_t action{};
  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_hex_hash(out->hashimyei, &normalized_hashimyei)) {
    out->error = "invalid wikimyei hashimyei id: " + out->hashimyei;
    return false;
  }
  std::filesystem::path report_fragment_directory{};
  if (!resolve_wikimyei_representation_encoding_vicreg_directory(
          normalized_hashimyei, false, &report_fragment_directory,
          &out->error)) {
    return false;
  }
  out->hashimyei = normalized_hashimyei;
  out->report_fragment_directory = report_fragment_directory;
  action.family_canonical_path =
      std::string(kWikimyeiEncodingVicregCanonicalType);
  action.family = std::string(kWikimyeiEncodingVicregFamily);
  action.model = std::string(kWikimyeiEncodingVicregModel);
  action.report_fragment_id = out->hashimyei;
  action.report_fragment_directory = out->report_fragment_directory;
  action.canonical_action = std::move(canonical_action);
  action.object_handle = model;
  action.user_data = out;

  std::string dispatch_error;
  if (!cuwacunu::hashimyei::dispatch_report_fragment_save(
          action.family_canonical_path, action, &dispatch_error)) {
    out->error = dispatch_error;
    return false;
  }

  out->ok = true;
  return true;
}

[[nodiscard]] inline wikimyei_representation_encoding_vicreg_init_record_t
persist_wikimyei_representation_encoding_vicreg_init(
    cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *model = nullptr) {
  namespace fs = std::filesystem;
  wikimyei_representation_encoding_vicreg_init_record_t out{};

  out.store_root = wikimyei_representation_encoding_vicreg_store_root();

  std::error_code ec;
  fs::create_directories(out.store_root, ec);
  if (ec) {
    out.error = "cannot create wikimyei report_fragment root: " +
                out.store_root.string();
    return out;
  }

  out.hashimyei =
      next_wikimyei_representation_encoding_vicreg_hash(out.store_root);
  out.report_fragment_directory = out.store_root / out.hashimyei;

  fs::create_directories(out.report_fragment_directory, ec);
  if (ec) {
    out.error = "cannot create wikimyei report_fragment directory: " +
                out.report_fragment_directory.string();
    return out;
  }

  if (!write_wikimyei_representation_encoding_vicreg_report_fragment_payload(
          "tsi.wikimyei.representation.encoding.vicreg.init()", &out, model)) {
    return out;
  }

  return out;
}

[[nodiscard]] inline wikimyei_representation_encoding_vicreg_init_record_t
update_wikimyei_representation_encoding_vicreg_init(
    std::string hashimyei,
    cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 *model,
    bool enable_network_analytics_sidecar,
    bool enable_embedding_sequence_analytics_sidecar, std::string contract_hash,
    std::string binding_id, std::string run_id,
    std::string source_runtime_cursor, bool has_wave_cursor,
    std::uint64_t wave_cursor,
    const cuwacunu::piaabo::torch_compat::sequence_analytics_report_t
        *embedding_sequence_analytics_report,
    const cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_report_t
        *embedding_sequence_symbolic_analytics_report,
    const vicreg_train_summary_snapshot_t *train_summary_snapshot,
    cuwacunu::piaabo::torch_compat::data_analytics_options_t
        embedding_sequence_analytics_options,
    const TsiWikimyeiRuntimeIoContext *runtime_io_context) {
  namespace fs = std::filesystem;
  wikimyei_representation_encoding_vicreg_init_record_t out{};
  out.store_root = wikimyei_representation_encoding_vicreg_store_root();
  out.enable_network_analytics_sidecar = enable_network_analytics_sidecar;
  out.enable_embedding_sequence_analytics_sidecar =
      enable_embedding_sequence_analytics_sidecar;
  out.contract_hash = std::move(contract_hash);
  out.binding_id = std::move(binding_id);
  out.run_id = std::move(run_id);
  out.source_runtime_cursor = std::move(source_runtime_cursor);
  out.has_wave_cursor = has_wave_cursor;
  out.wave_cursor = wave_cursor;
  if (runtime_io_context) {
    out.component_tag = runtime_io_context->component_tag;
    out.component_compatibility_sha256_hex =
        runtime_io_context->component_compatibility_sha256_hex;
    out.docking_signature_sha256_hex =
        runtime_io_context->docking_signature_sha256_hex;
    out.instrument_signature = runtime_io_context->instrument_signature;
    out.runtime_instrument_signature =
        runtime_io_context->runtime_instrument_signature;
  }
  out.embedding_sequence_analytics_options =
      embedding_sequence_analytics_options;
  if (embedding_sequence_analytics_report &&
      embedding_sequence_symbolic_analytics_report) {
    out.has_embedding_sequence_analytics = true;
    out.embedding_sequence_analytics_report =
        *embedding_sequence_analytics_report;
    out.embedding_sequence_symbolic_analytics_report =
        *embedding_sequence_symbolic_analytics_report;
  }
  if (train_summary_snapshot) {
    out.has_train_summary = true;
    out.train_summary_snapshot = *train_summary_snapshot;
  }

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_hex_hash(hashimyei, &normalized_hashimyei)) {
    out.error = "invalid wikimyei hashimyei id: " + hashimyei;
    return out;
  }

  out.hashimyei = std::move(normalized_hashimyei);
  if (!resolve_wikimyei_representation_encoding_vicreg_directory(
          out.hashimyei, false, &out.report_fragment_directory, &out.error)) {
    return out;
  }

  std::error_code ec;
  if (!fs::exists(out.report_fragment_directory, ec)) {
    fs::create_directories(out.report_fragment_directory, ec);
    if (ec) {
      out.error = "cannot create wikimyei report_fragment directory: " +
                  out.report_fragment_directory.string();
      return out;
    }
  } else if (!fs::is_directory(out.report_fragment_directory, ec)) {
    out.error = "wikimyei report_fragment path is not a directory: " +
                out.report_fragment_directory.string();
    return out;
  }

  if (!write_wikimyei_representation_encoding_vicreg_report_fragment_payload(
          "tsi.wikimyei.representation.encoding.vicreg.edit()", &out, model)) {
    return out;
  }

  return out;
}

[[nodiscard]] inline bool
load_wikimyei_representation_encoding_vicreg_init_into_model(
    std::string_view hashimyei, const std::string &contract_hash,
    const std::string &component_name, torch::Device preferred_device,
    vicreg_public_docking_shape_t expected_public_docking,
    std::unique_ptr<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4> *model_slot,
    std::string *error) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();
  if (!model_slot) {
    if (error)
      *error = "vicreg model slot is null";
    return false;
  }

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_hex_hash(hashimyei, &normalized_hashimyei)) {
    if (error)
      *error = "invalid wikimyei hashimyei id: " + std::string(hashimyei);
    return false;
  }

  fs::path report_fragment_directory{};
  if (!resolve_wikimyei_representation_encoding_vicreg_directory(
          normalized_hashimyei, true, &report_fragment_directory, error)) {
    return false;
  }

  const fs::path weights_file = report_fragment_directory / "weights.init.pt";

  if (cuwacunu::hashimyei::report_fragment_manifest_exists(
          report_fragment_directory)) {
    cuwacunu::hashimyei::report_fragment_manifest_t manifest{};
    std::string manifest_error;
    if (!cuwacunu::hashimyei::read_report_fragment_manifest(
            report_fragment_directory, &manifest, &manifest_error)) {
      if (error)
        *error = "cannot read report_fragment manifest: " + manifest_error;
      return false;
    }
    if (manifest.family_canonical_path !=
        std::string(kWikimyeiEncodingVicregCanonicalType)) {
      if (error) {
        *error = "report_fragment manifest family_canonical_path mismatch: " +
                 manifest.family_canonical_path;
      }
      return false;
    }
    if (manifest.report_fragment_id != normalized_hashimyei) {
      if (error) {
        *error = "report_fragment manifest hashimyei mismatch: " +
                 manifest.report_fragment_id;
      }
      return false;
    }
    if (!cuwacunu::hashimyei::report_fragment_manifest_has_file(
            manifest, weights_file.filename().string())) {
      if (error) {
        *error = "report_fragment manifest missing weights file entry: " +
                 weights_file.filename().string();
      }
      return false;
    }
  }

  auto public_docking_matches =
      [&](const cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 &candidate)
      -> bool {
    if (candidate.C != expected_public_docking.C ||
        candidate.T != expected_public_docking.T ||
        candidate.D != expected_public_docking.D ||
        candidate.encoding_dims != expected_public_docking.encoding_dims) {
      if (error) {
        std::ostringstream oss;
        oss << "vicreg public docking mismatch: expected C/Hx/Dx/De=("
            << expected_public_docking.C << "," << expected_public_docking.T
            << "," << expected_public_docking.D << ","
            << expected_public_docking.encoding_dims << "), got ("
            << candidate.C << "," << candidate.T << "," << candidate.D << ","
            << candidate.encoding_dims << ")";
        *error = oss.str();
      }
      return false;
    }
    return true;
  };

  std::unique_ptr<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4> replacement{};
  try {
    replacement =
        std::make_unique<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4>(
            contract_hash, component_name, weights_file.string(),
            preferred_device);
  } catch (const std::exception &e) {
    const std::string load_error = std::string(e.what());
    const bool cuda_runtime_only_failure =
        load_error.find("cudaGetDeviceCount") != std::string::npos ||
        load_error.find("cuda unavailable") != std::string::npos ||
        load_error.find("operation not supported on this OS") !=
            std::string::npos;
    if (cuda_runtime_only_failure && !preferred_device.is_cuda()) {
      log_warn("[tsi.vicreg] report_fragment load skipped (cuda runtime "
               "unavailable). "
               "Using fresh-initialized model for this run.\n");
      if (error)
        error->clear();
      return true;
    }
    if (error)
      *error = "vicreg load failed: " + load_error;
    return false;
  }

  if (!public_docking_matches(*replacement)) {
    return false;
  }

  *model_slot = std::move(replacement);
  return true;
}

[[nodiscard]] inline bool delete_wikimyei_representation_encoding_vicreg_init(
    std::string_view hashimyei, std::uintmax_t *removed_count = nullptr,
    std::string *error = nullptr) {
  namespace fs = std::filesystem;
  if (removed_count)
    *removed_count = 0;
  if (error)
    error->clear();

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_hex_hash(hashimyei, &normalized_hashimyei)) {
    if (error)
      *error = "invalid wikimyei hashimyei id";
    return false;
  }

  fs::path target{};
  if (!resolve_wikimyei_representation_encoding_vicreg_directory(
          normalized_hashimyei, true, &target, error)) {
    return false;
  }

  std::error_code ec;
  const std::uintmax_t removed = fs::remove_all(target, ec);
  if (ec) {
    if (error)
      *error = "failed to delete wikimyei report_fragment: " + target.string();
    return false;
  }

  if (removed_count)
    *removed_count = removed;
  return true;
}

[[nodiscard]] inline wikimyei_representation_encoding_vicreg_init_record_t
invoke_wikimyei_representation_encoding_vicreg_init_from_config() {
  return persist_wikimyei_representation_encoding_vicreg_init();
}

} // namespace tsiemene
