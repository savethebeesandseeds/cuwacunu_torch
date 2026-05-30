private:
[[nodiscard]] static std::string build_transfer_matrix_report_canonical_path_(
    std::string_view representation_hashimyei) {
  std::string out("tsi.wikimyei.representation.encoding.vicreg");
  if (!has_non_ws_ascii_(representation_hashimyei))
    return out;
  out.push_back('.');
  out.append(representation_hashimyei);
  return out;
}

[[nodiscard]] std::filesystem::path build_transfer_matrix_runtime_report_path_(
    std::string_view canonical_path) const {
  const std::string contract_token =
      cuwacunu::hashimyei::compact_contract_hash_path_token(contract_hash_);
  return cuwacunu::hashimyei::canonical_path_directory(
             cuwacunu::hashimyei::store_root(), canonical_path) /
         "contracts" / contract_token /
         "transfer_matrix_evaluation.summary.latest.lls";
}

[[nodiscard]] std::filesystem::path
build_transfer_matrix_runtime_matrix_report_path_(
    std::string_view canonical_path) const {
  const std::string contract_token =
      cuwacunu::hashimyei::compact_contract_hash_path_token(contract_hash_);
  return cuwacunu::hashimyei::canonical_path_directory(
             cuwacunu::hashimyei::store_root(), canonical_path) /
         "contracts" / contract_token /
         "transfer_matrix_evaluation.matrix.latest.lls";
}

void persist_transfer_matrix_run_report_(const RuntimeEvent &event,
                                         Emitter &out) const {
  if (!transfer_matrix_evaluator_)
    return;
  std::string report_lls = transfer_matrix_evaluator_->last_run_report_lls();
  std::string matrix_report_lls =
      transfer_matrix_evaluator_->last_run_matrix_report_lls();
  if (!has_non_ws_ascii_(report_lls) && !has_non_ws_ascii_(matrix_report_lls)) {
    return;
  }

  const std::string canonical_path =
      build_transfer_matrix_report_canonical_path_(representation_hashimyei_);
  const std::filesystem::path report_file =
      build_transfer_matrix_runtime_report_path_(canonical_path);
  const std::filesystem::path matrix_report_file =
      build_transfer_matrix_runtime_matrix_report_path_(canonical_path);

  std::error_code ec;
  std::filesystem::create_directories(report_file.parent_path(), ec);
  if (ec) {
    log_warn(
        "[tsi.vicreg] transfer-matrix report directory create failed: %s\n",
        report_file.parent_path().string().c_str());
    if (event.wave) {
      emit_meta_(*event.wave, out,
                 "transfer_matrix_eval.report_save_failed reason=create_dir");
    }
    return;
  }

  const auto write_payload = [&](const std::filesystem::path &path,
                                 std::string_view semantic_taxon,
                                 std::string payload_lls) -> bool {
    if (!has_non_ws_ascii_(payload_lls))
      return true;
    runtime_lls_document_t payload{};
    std::string parse_error;
    if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
            payload_lls, &payload, &parse_error)) {
      log_warn("[tsi.vicreg] transfer-matrix report parse failed: %s (%s)\n",
               path.string().c_str(), parse_error.c_str());
      return false;
    }
    auto report_identity = make_component_report_identity(
        canonical_path, runtime_binding_id_, semantic_taxon);
    report_identity.source_runtime_cursor = runtime_source_runtime_cursor_;
    report_identity.has_wave_cursor = runtime_has_wave_cursor_;
    report_identity.wave_cursor = runtime_wave_cursor_;
    cuwacunu::piaabo::latent_lineage_state::
        append_runtime_report_header_entries(
            &payload, make_runtime_report_header(report_identity));
    append_runtime_string_entry_if_nonempty_(
        &payload, "component_instance_name", component_name_);
    append_runtime_string_entry_(&payload, "report_event",
                                 report_event_token(report_event_e::RunEnd));

    std::string canonical_payload;
    try {
      canonical_payload =
          cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
              payload);
    } catch (const std::exception &e) {
      log_warn("[tsi.vicreg] transfer-matrix report emit failed: %s (%s)\n",
               path.string().c_str(), e.what());
      return false;
    }

    std::ofstream out_file(path, std::ios::binary | std::ios::trunc);
    if (!out_file) {
      log_warn("[tsi.vicreg] transfer-matrix report open failed: %s\n",
               path.string().c_str());
      return false;
    }
    out_file << canonical_payload;
    if (!out_file.good()) {
      log_warn("[tsi.vicreg] transfer-matrix report write failed: %s\n",
               path.string().c_str());
      return false;
    }
    return true;
  };

  const bool has_summary_payload = has_non_ws_ascii_(report_lls);
  const bool has_matrix_payload = has_non_ws_ascii_(matrix_report_lls);
  const bool wrote_summary =
      !has_summary_payload ||
      write_payload(report_file,
                    cuwacunu::piaabo::latent_lineage_state::report_taxonomy::
                        kEmbeddingEvaluation,
                    std::move(report_lls));
  const bool wrote_matrix =
      !has_matrix_payload ||
      write_payload(matrix_report_file,
                    cuwacunu::piaabo::latent_lineage_state::report_taxonomy::
                        kEmbeddingEvaluation,
                    std::move(matrix_report_lls));
  if (!wrote_summary || !wrote_matrix) {
    if (event.wave) {
      emit_meta_(*event.wave, out,
                 "transfer_matrix_eval.report_save_failed reason=write_file");
    }
    return;
  }
  if (event.wave) {
    std::string msg = std::string("transfer_matrix_eval.report_saved path=") +
                      report_file.string();
    if (has_matrix_payload && wrote_matrix) {
      msg += " matrix_path=" + matrix_report_file.string();
    }
    emit_meta_(*event.wave, out, std::move(msg));
  }
}

void apply_epoch_training_policy_() {
  if (epoch_optimizer_steps_ <= 0)
    return;
  ++model().runtime_state.trained_epochs;

  if (model().optimizer_threshold_reset >= 0 && model().optimizer) {
    cuwacunu::jkimyei::optim::clamp_adam_step(
        *model().optimizer,
        static_cast<int64_t>(model().optimizer_threshold_reset));
  }

  const double metric =
      epoch_loss_sum_ / static_cast<double>(epoch_optimizer_steps_);
  if (model().lr_sched) {
    const auto mode = model().lr_sched->mode;
    if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch) {
      model().lr_sched->step();
    } else if (mode ==
               cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpochWithMetric) {
      model().lr_sched->step(metric);
    }
  }

  last_train_summary_ = vicreg_train_summary_snapshot_t{
      .epoch_index = model().runtime_state.trained_epochs,
      .optimizer_steps = static_cast<std::uint64_t>(
          std::max(0, model().runtime_state.optimizer_steps)),
      .trained_epochs = model().runtime_state.trained_epochs,
      .trained_samples = model().runtime_state.trained_samples,
      .skipped_batches = model().runtime_state.skipped_batches,
      .epoch_loss_mean = metric,
      .epoch_inv_mean =
          epoch_inv_sum_ / static_cast<double>(epoch_optimizer_steps_),
      .epoch_var_mean =
          epoch_var_sum_ / static_cast<double>(epoch_optimizer_steps_),
      .epoch_cov_raw_mean =
          epoch_cov_raw_sum_ / static_cast<double>(epoch_optimizer_steps_),
      .epoch_cov_weighted_mean =
          epoch_cov_weighted_sum_ / static_cast<double>(epoch_optimizer_steps_),
      .last_committed_loss_mean =
          model().runtime_state.last_committed_loss_mean,
      .learning_rate = current_optimizer_learning_rate_(),
  };
  has_last_train_summary_ = true;
  training_activity_this_run_ = true;
  reset_epoch_training_accumulators_();
}

[[nodiscard]] bool flush_pending_training_step_if_any_() {
  if (!model().finalize_pending_training_step(
          model().training_policy.swa_start_iter)) {
    return false;
  }
  note_committed_training_step_();
  return true;
}

void reset_epoch_training_accumulators_() {
  epoch_loss_sum_ = 0.0;
  epoch_inv_sum_ = 0.0;
  epoch_var_sum_ = 0.0;
  epoch_cov_raw_sum_ = 0.0;
  epoch_cov_weighted_sum_ = 0.0;
  epoch_optimizer_steps_ = 0;
}

void note_committed_training_step_() {
  epoch_loss_sum_ += model().runtime_state.last_committed_loss_mean;
  epoch_inv_sum_ += model().runtime_state.last_committed_inv_mean;
  epoch_var_sum_ += model().runtime_state.last_committed_var_mean;
  epoch_cov_raw_sum_ += model().runtime_state.last_committed_cov_raw_mean;
  epoch_cov_weighted_sum_ +=
      model().runtime_state.last_committed_cov_weighted_mean;
  ++epoch_optimizer_steps_;
  training_activity_this_run_ = true;
}

[[nodiscard]] double current_optimizer_learning_rate_() const {
  if (!model().optimizer)
    return 0.0;
  const auto &param_groups = model().optimizer->param_groups();
  if (param_groups.empty())
    return 0.0;
  return param_groups.front().options().get_lr();
}

void emit_epoch_training_trace_(const RuntimeContext &ctx) const {
  if (!has_last_train_summary_)
    return;
  emit_runtime_binding_trace_event(
      {.phase = "vicreg_epoch_summary",
       .binding_id = ctx.binding_id,
       .contract_hash = contract_hash_,
       .component_name = component_name_,
       .run_id = ctx.run_id,
       .source_runtime_cursor = ctx.source_runtime_cursor,
       .epochs = last_train_summary_.trained_epochs,
       .epoch = last_train_summary_.epoch_index,
       .optimizer_steps = last_train_summary_.optimizer_steps,
       .trained_epochs = last_train_summary_.trained_epochs,
       .trained_samples = last_train_summary_.trained_samples,
       .skipped_batches = last_train_summary_.skipped_batches,
       .epoch_loss_mean = last_train_summary_.epoch_loss_mean,
       .last_committed_loss_mean = last_train_summary_.last_committed_loss_mean,
       .loss_inv_mean = last_train_summary_.epoch_inv_mean,
       .loss_var_mean = last_train_summary_.epoch_var_mean,
       .loss_cov_raw_mean = last_train_summary_.epoch_cov_raw_mean,
       .loss_cov_weighted_mean = last_train_summary_.epoch_cov_weighted_mean,
       .learning_rate = last_train_summary_.learning_rate,
       .ok = true});
}

void apply_runtime_policy_from_jkimyei_(bool requested_train,
                                        bool requested_use_swa,
                                        bool requested_detach_to_cpu) {
  const bool jk_use_swa = model().training_policy.vicreg_use_swa;
  const bool jk_detach_to_cpu = model().training_policy.vicreg_detach_to_cpu;

  if (requested_use_swa != jk_use_swa ||
      requested_detach_to_cpu != jk_detach_to_cpu) {
    log_warn("[tsi.vicreg] runtime flags (%s/%s) differ from jkimyei policy "
             "(%s/%s) for component '%s'; using jkimyei policy for non-train "
             "flags\n",
             requested_use_swa ? "swa" : "base",
             requested_detach_to_cpu ? "detach" : "keep_device",
             jk_use_swa ? "swa" : "base",
             jk_detach_to_cpu ? "detach" : "keep_device",
             component_name_.c_str());
  }

  train_ = requested_train;
  use_swa_ = jk_use_swa;
  detach_to_cpu_ = jk_detach_to_cpu;
}

void maybe_initialize_embedding_sequence_evaluator_() {
  try {
    embedding_sequence_evaluator_ =
        std::make_unique<VicregEmbeddingSequenceEvaluator>(
            contract_hash_, model().encoding_dims);
  } catch (const std::exception &e) {
    log_warn(
        "[tsi.vicreg] embedding-sequence evaluator disabled for '%s': %s\n",
        component_name_.c_str(), e.what());
    embedding_sequence_evaluator_.reset();
  } catch (...) {
    log_warn("[tsi.vicreg] embedding-sequence evaluator disabled for '%s': "
             "unknown error\n",
             component_name_.c_str());
    embedding_sequence_evaluator_.reset();
  }
}

void maybe_initialize_transfer_matrix_evaluator_() {
  try {
    transfer_matrix_evaluator_ =
        std::make_unique<VicregTransferMatrixEvaluator>(
            contract_hash_,
            VicregTransferMatrixEvaluator::default_evaluation_policy());
  } catch (const std::exception &e) {
    log_warn("[tsi.vicreg] transfer-matrix evaluator disabled for '%s': %s\n",
             component_name_.c_str(), e.what());
    transfer_matrix_evaluator_.reset();
  } catch (...) {
    log_warn("[tsi.vicreg] transfer-matrix evaluator disabled for '%s': "
             "unknown error\n",
             component_name_.c_str());
    transfer_matrix_evaluator_.reset();
  }
}

static std::string tensor_shape_(const torch::Tensor &t) {
  if (!t.defined())
    return ":tensor undefined";
  std::ostringstream oss;
  oss << ":tensor shape=[";
  const auto sizes = t.sizes();
  for (std::size_t i = 0; i < sizes.size(); ++i) {
    if (i > 0)
      oss << ",";
    oss << sizes[i];
  }
  oss << "]";
  return oss.str();
}

static std::string signal_shape_(const Signal &s) {
  if (s.kind == PayloadKind::Cargo) {
    if (!s.cargo)
      return ":cargo null";
    const auto &c = *s.cargo;
    std::ostringstream oss;
    oss << ":cargo features=" << tensor_shape_(c.features)
        << " mask=" << tensor_shape_(c.mask)
        << " encoding=" << tensor_shape_(c.encoding);
    return oss.str();
  }
  if (s.kind == PayloadKind::String) {
    return ":str bytes=" + std::to_string(s.text.size());
  }
  return tensor_shape_(s.tensor);
}

void emit_meta_(const Wave &wave, Emitter &out, std::string msg) const {
  out.emit_string(wave, OUT_META, std::move(msg));
}

void capture_runtime_report_context_(const Wave *wave,
                                     const RuntimeContext &ctx) {
  if (!ctx.binding_id.empty())
    runtime_binding_id_ = ctx.binding_id;
  if (!ctx.run_id.empty())
    runtime_run_id_ = ctx.run_id;
  if (!ctx.source_runtime_cursor.empty()) {
    runtime_source_runtime_cursor_ = ctx.source_runtime_cursor;
  }
  if (ctx.has_wave_cursor) {
    runtime_has_wave_cursor_ = true;
    runtime_wave_cursor_ = ctx.wave_cursor;
    runtime_wave_cursor_run_ = 0;
    runtime_wave_cursor_episode_ = 0;
    runtime_wave_cursor_batch_ = 0;
    return;
  }
  if (!wave)
    return;
  std::uint64_t packed_wave_cursor = 0;
  if (!cuwacunu::piaabo::latent_lineage_state::pack_runtime_wave_cursor(
          wave->cursor.i, wave->cursor.episode, wave->cursor.batch,
          &packed_wave_cursor)) {
    return;
  }
  runtime_has_wave_cursor_ = true;
  runtime_wave_cursor_ = packed_wave_cursor;
  runtime_wave_cursor_run_ = wave->cursor.i;
  runtime_wave_cursor_episode_ = wave->cursor.episode;
  runtime_wave_cursor_batch_ = wave->cursor.batch;
}

TsiId id_{};
std::string instance_name_;
std::string contract_hash_;
std::string runtime_binding_id_{};
std::string runtime_run_id_{};
std::string runtime_source_runtime_cursor_{};
bool runtime_has_wave_cursor_{false};
std::uint64_t runtime_wave_cursor_{0};
std::uint64_t runtime_wave_cursor_run_{0};
std::uint64_t runtime_wave_cursor_episode_{0};
std::uint64_t runtime_wave_cursor_batch_{0};
std::string representation_hashimyei_;
std::string component_name_;

bool train_{false};
bool use_swa_{true};
bool detach_to_cpu_{true};
double epoch_loss_sum_{0.0};
double epoch_inv_sum_{0.0};
double epoch_var_sum_{0.0};
double epoch_cov_raw_sum_{0.0};
double epoch_cov_weighted_sum_{0.0};
int epoch_optimizer_steps_{0};
bool has_last_train_summary_{false};
bool training_activity_this_run_{false};
vicreg_train_summary_snapshot_t last_train_summary_{};
std::unique_ptr<VicregEmbeddingSequenceEvaluator>
    embedding_sequence_evaluator_{};
std::unique_ptr<VicregTransferMatrixEvaluator> transfer_matrix_evaluator_{};

[[nodiscard]] cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 &model() noexcept {
  return *model_;
}
[[nodiscard]] const cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4 &
model() const noexcept {
  return *model_;
}

std::unique_ptr<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4> model_;
}
;
