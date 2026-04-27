class TsiWikimyeiRepresentationEncodingVicreg final
    : public TsiWikimyeiRepresentation {
public:
  static constexpr DirectiveId IN_STEP = directive_id::Step;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_LOSS = directive_id::Loss;
  static constexpr DirectiveId OUT_META = directive_id::Meta;

  TsiWikimyeiRepresentationEncodingVicreg(TsiId id, std::string instance_name,
                                          std::string contract_hash,
                                          std::string representation_hashimyei,
                                          std::string component_name, int C,
                                          int T, int D, bool train = false,
                                          bool use_swa = true,
                                          bool detach_to_cpu = true)
      : id_(id), instance_name_(std::move(instance_name)),
        contract_hash_(std::move(contract_hash)),
        representation_hashimyei_(std::move(representation_hashimyei)),
        component_name_(std::move(component_name)),
        model_(std::make_unique<cuwacunu::wikimyei::vicreg_rank4::VICReg_Rank4>(
            contract_hash_, component_name_, C, T, D)) {
    apply_runtime_policy_from_jkimyei_(train, use_swa, detach_to_cpu);
    maybe_initialize_embedding_sequence_evaluator_();
    maybe_initialize_transfer_matrix_evaluator_();
  }

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.wikimyei.representation.encoding.vicreg";
  }
  [[nodiscard]] std::string_view instance_name() const noexcept override {
    return instance_name_;
  }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }
  [[nodiscard]] bool emits_loss_directive() const noexcept override {
    return true;
  }
  [[nodiscard]] bool supports_jkimyei_facet() const noexcept override {
    return true;
  }
  [[nodiscard]] bool supports_init_report_fragments() const noexcept override {
    return true;
  }
  [[nodiscard]] bool
  runtime_autosave_report_fragments() const noexcept override {
    return train_;
  }
  [[nodiscard]] bool
  runtime_load_from_hashimyei(std::string_view hashimyei,
                              std::string *error = nullptr,
                              const void *runtime_context = nullptr) override {
    (void)runtime_context;
    const auto expected_public_docking =
        capture_vicreg_public_docking_shape_(model());
    const bool loaded =
        load_wikimyei_representation_encoding_vicreg_init_into_model(
            hashimyei, contract_hash_, component_name_, model().device,
            expected_public_docking, &model_, error);
    if (loaded) {
      apply_runtime_policy_from_jkimyei_(train_, use_swa_, detach_to_cpu_);
      reset_epoch_training_accumulators_();
      has_last_train_summary_ = false;
      training_activity_this_run_ = false;
    }
    if (loaded && embedding_sequence_evaluator_) {
      embedding_sequence_evaluator_->reset();
    }
    return loaded;
  }
  [[nodiscard]] bool
  runtime_save_to_hashimyei(std::string_view hashimyei,
                            std::string *error = nullptr,
                            const void *runtime_context = nullptr) override {
    // Runtime Hero serializes jobs that target the same mutable
    // representation_type.hashimyei identity, so in-place report-fragment
    // publication remains single-writer at the wikimyei boundary.
    bool enable_network_analytics_sidecar = true;
    bool enable_embedding_sequence_analytics_sidecar = true;
    const TsiWikimyeiRuntimeIoContext *runtime_io_ctx = nullptr;
    if (runtime_context) {
      runtime_io_ctx =
          static_cast<const TsiWikimyeiRuntimeIoContext *>(runtime_context);
      enable_network_analytics_sidecar = runtime_io_ctx->enable_debug_outputs;
      enable_embedding_sequence_analytics_sidecar =
          runtime_io_ctx->enable_debug_outputs;
      if (!runtime_io_ctx->binding_id.empty()) {
        runtime_binding_id_ = runtime_io_ctx->binding_id;
      }
      if (!runtime_io_ctx->run_id.empty()) {
        runtime_run_id_ = runtime_io_ctx->run_id;
      }
      if (!runtime_io_ctx->source_runtime_cursor.empty()) {
        runtime_source_runtime_cursor_ = runtime_io_ctx->source_runtime_cursor;
      }
      if (runtime_io_ctx->has_wave_cursor) {
        runtime_has_wave_cursor_ = true;
        runtime_wave_cursor_ = runtime_io_ctx->wave_cursor;
      }
    }
    const auto embedding_sequence_snapshot =
        embedding_sequence_evaluator_
            ? embedding_sequence_evaluator_->snapshot()
            : std::nullopt;
    const vicreg_train_summary_snapshot_t *train_summary_snapshot =
        (training_activity_this_run_ && has_last_train_summary_)
            ? &last_train_summary_
            : nullptr;
    auto out = update_wikimyei_representation_encoding_vicreg_init(
        std::string(hashimyei), model_.get(), enable_network_analytics_sidecar,
        enable_embedding_sequence_analytics_sidecar, contract_hash_,
        runtime_binding_id_, runtime_run_id_, runtime_source_runtime_cursor_,
        runtime_has_wave_cursor_, runtime_wave_cursor_,
        embedding_sequence_snapshot ? &embedding_sequence_snapshot->report
                                    : nullptr,
        embedding_sequence_snapshot
            ? &embedding_sequence_snapshot->symbolic_report
            : nullptr,
        train_summary_snapshot,
        embedding_sequence_snapshot
            ? embedding_sequence_snapshot->options
            : cuwacunu::piaabo::torch_compat::data_analytics_options_t{},
        runtime_io_ctx);
    if (out.ok)
      return true;
    if (error)
      *error = out.error;
    return false;
  }
  [[nodiscard]] bool train_enabled() const noexcept { return train_; }
  [[nodiscard]] int optimizer_steps() const noexcept {
    return model().runtime_optimizer_steps();
  }
  [[nodiscard]] bool
  jkimyei_get_runtime_state(TsiWikimyeiJkimyeiRuntimeState *out,
                            std::string *error = nullptr) const override {
    if (error)
      error->clear();
    if (!out) {
      if (error)
        *error = "jkimyei runtime state output pointer is null";
      return false;
    }

    *out = TsiWikimyeiJkimyeiRuntimeState{};
    out->runtime_component_name = component_name_;
    out->run_id = runtime_run_id_;
    out->train_enabled = train_;
    out->use_swa = use_swa_;
    out->detach_to_cpu = detach_to_cpu_;
    out->supports_runtime_profile_switch = false;
    out->has_pending_grad = model().runtime_state.has_pending_grad;
    out->skip_on_nan = model().training_policy.skip_on_nan;
    out->zero_grad_set_to_none = model().training_policy.zero_grad_set_to_none;
    out->optimizer_steps = model().runtime_state.optimizer_steps;
    out->accumulate_steps = model().training_policy.accumulate_steps;
    out->accum_counter = model().runtime_state.accum_counter;
    out->swa_start_iter = model().training_policy.swa_start_iter;
    out->trained_epochs = model().runtime_state.trained_epochs;
    out->trained_samples = model().runtime_state.trained_samples;
    out->skipped_batches = model().runtime_state.skipped_batches;
    out->clip_norm = model().training_policy.clip_norm;
    out->clip_value = model().training_policy.clip_value;
    out->last_committed_loss_mean =
        model().runtime_state.last_committed_loss_mean;
    out->last_committed_inv_mean =
        model().runtime_state.last_committed_inv_mean;
    out->last_committed_var_mean =
        model().runtime_state.last_committed_var_mean;
    out->last_committed_cov_raw_mean =
        model().runtime_state.last_committed_cov_raw_mean;
    out->last_committed_cov_weighted_mean =
        model().runtime_state.last_committed_cov_weighted_mean;

    try {
      const auto &jk_component =
          cuwacunu::jkimyei::jk_setup(component_name_, contract_hash_);
      out->resolved_component_id = jk_component.resolved_component_id;
      out->profile_id = jk_component.resolved_profile_id;
      out->profile_row_id = jk_component.resolved_profile_row_id;
    } catch (const std::exception &e) {
      if (error) {
        *error =
            std::string("failed to resolve jkimyei runtime state: ") + e.what();
      }
      return false;
    } catch (...) {
      if (error) {
        *error = "failed to resolve jkimyei runtime state: unknown exception";
      }
      return false;
    }

    return true;
  }
  [[nodiscard]] bool
  jkimyei_set_train_enabled(bool enabled,
                            std::string *error = nullptr) override {
    if (error)
      error->clear();
    set_train(enabled);
    return true;
  }
  [[nodiscard]] bool jkimyei_flush_pending_training_step(
      TsiWikimyeiJkimyeiFlushResult *out = nullptr,
      std::string *error = nullptr) override {
    if (error)
      error->clear();
    if (out)
      *out = TsiWikimyeiJkimyeiFlushResult{};

    const bool had_pending_grad = model().runtime_state.has_pending_grad;
    bool optimizer_step_applied = false;
    try {
      optimizer_step_applied = flush_pending_training_step_if_any_();
    } catch (const std::exception &e) {
      if (error) {
        *error =
            std::string("failed to flush pending training step: ") + e.what();
      }
      return false;
    } catch (...) {
      if (error) {
        *error = "failed to flush pending training step: unknown exception";
      }
      return false;
    }

    if (out) {
      out->had_pending_grad = had_pending_grad;
      out->optimizer_step_applied = optimizer_step_applied;
      out->has_pending_grad_after = model().runtime_state.has_pending_grad;
      out->last_committed_loss_mean =
          model().runtime_state.last_committed_loss_mean;
    }
    return true;
  }
  [[nodiscard]] std::string_view
  init_report_fragment_schema() const noexcept override {
    return "tsi.wikimyei.representation.encoding.vicreg.init.v1";
  }
  [[nodiscard]] std::string_view
  report_fragment_family() const noexcept override {
    return "representation";
  }
  [[nodiscard]] std::string_view
  report_fragment_model() const noexcept override {
    return "vicreg";
  }

  [[nodiscard]] std::span<const DirectiveSpec>
  directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
        directive(
            IN_STEP, DirectiveDir::In, KindSpec::Cargo(),
            "observation cargo (features/mask/future/keys/stats/encoding)"),
        directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::Cargo(),
                  "observation cargo with representation encoding"),
        directive(OUT_LOSS, DirectiveDir::Out, KindSpec::Tensor(),
                  "loss scalar (only when train=true)"),
        directive(OUT_META, DirectiveDir::Out, KindSpec::String(),
                  "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 4);
  }

  void set_train(bool on) noexcept {
    if (train_ && !on) {
      try {
        (void)flush_pending_training_step_if_any_();
        apply_epoch_training_policy_();
      } catch (const std::exception &e) {
        log_warn("[tsi.vicreg] set_train(false) flush failed: %s\n", e.what());
      } catch (...) {
        log_warn(
            "[tsi.vicreg] set_train(false) flush failed with unknown error\n");
      }
    }
    train_ = on;
  }

  void step(const Wave &wave, Ingress in, RuntimeContext &ctx,
            Emitter &out) override {
    if (in.directive != IN_STEP)
      return;
    if (in.signal.kind != PayloadKind::Cargo)
      return;
    capture_runtime_report_context_(&wave, ctx);

    {
      std::ostringstream oss;
      oss << "vicreg.in kind=" << kind_token(in.signal.kind)
          << " signal=" << signal_shape_(in.signal)
          << " train=" << (train_ ? "true" : "false")
          << " use_swa=" << (use_swa_ ? "true" : "false")
          << " detach_to_cpu=" << (detach_to_cpu_ ? "true" : "false");
      emit_meta_(wave, out, oss.str());
    }

    using sample_t = cuwacunu::camahjucunu::data::observation_sample_t;
    std::string cargo_error;
    if (!validate_observation_cargo(
            in.signal.cargo, CargoValidationStage::VicregIn, &cargo_error)) {
      emit_meta_(wave, out,
                 std::string("cargo.invalid stage=vicreg.in reason=") +
                     cargo_error);
      return;
    }
    const auto &sample = *in.signal.cargo;
    torch::Tensor data = sample.features;
    torch::Tensor mask = sample.mask;
    std::shared_ptr<sample_t> out_sample = std::make_shared<sample_t>(sample);

    data = data.to(model().device);
    mask = mask.to(model().device);

    // Always emit representation cargo.
    auto repr = model().encode(data, mask, /*use_swa=*/use_swa_,
                               /*detach_to_cpu=*/detach_to_cpu_);
    if (detach_to_cpu_)
      repr = repr.cpu();
    if (ctx.debug_enabled && embedding_sequence_evaluator_) {
      embedding_sequence_evaluator_->ingest_encoding(repr, sample.mask);
    }
    out_sample->encoding = repr;
    if (!validate_observation_cargo(out_sample, CargoValidationStage::VicregOut,
                                    &cargo_error)) {
      emit_meta_(wave, out,
                 std::string("cargo.invalid stage=vicreg.out reason=") +
                     cargo_error);
      return;
    }
    if (transfer_matrix_evaluator_) {
      transfer_matrix_evaluator_->ingest_representation_output(
          wave, out_sample.get(), ctx, out);
    }
    const std::string repr_shape = tensor_shape_(repr);
    out.emit_cargo(wave, OUT_PAYLOAD, std::move(out_sample));
    emit_meta_(wave, out,
               std::string("vicreg.out payload=encoding") + repr_shape);

    if (train_) {
      auto step_result = model().train_one_batch(
          data, mask, model().training_policy.swa_start_iter,
          /*verbose=*/false);
      if (step_result.loss.defined()) {
        TORCH_CHECK(torch::isfinite(step_result.loss).all().item<bool>(),
                    "[tsi.vicreg] training loss is non-finite");
        auto loss = step_result.loss.detach();
        if (detach_to_cpu_)
          loss = loss.to(torch::kCPU);
        const std::string loss_shape = tensor_shape_(loss);
        out.emit_tensor(wave, OUT_LOSS, std::move(loss));
        emit_meta_(wave, out,
                   std::string("vicreg.out loss=") + loss_shape +
                       " optimizer_step=" +
                       (step_result.optimizer_step_applied ? "true" : "false"));
        if (step_result.optimizer_step_applied) {
          note_committed_training_step_();
        }
      } else {
        emit_meta_(wave, out, "vicreg.out loss=skipped");
      }
    }
  }

  RuntimeEventAction on_event(const RuntimeEvent &event, RuntimeContext &ctx,
                              Emitter &out) override {
    capture_runtime_report_context_(event.wave, ctx);
    if (transfer_matrix_evaluator_) {
      transfer_matrix_evaluator_->handle_runtime_event(event, ctx, out);
    }
    switch (event.kind) {
    case RuntimeEventKind::RunStart: {
      if (embedding_sequence_evaluator_) {
        embedding_sequence_evaluator_->reset();
      }
      reset_epoch_training_accumulators_();
      has_last_train_summary_ = false;
      training_activity_this_run_ = false;
      return RuntimeEventAction{};
    }
    case RuntimeEventKind::RunEnd: {
      if (ctx.debug_enabled) {
        persist_transfer_matrix_run_report_(event, out);
      }
      return RuntimeEventAction{};
    }
    case RuntimeEventKind::EpochEnd: {
      if (!train_)
        return RuntimeEventAction{};
      (void)flush_pending_training_step_if_any_();
      apply_epoch_training_policy_();
      emit_epoch_training_trace_(ctx);
      if (!ctx.debug_enabled)
        return RuntimeEventAction{};
      vicreg_network_analytics_plan_t plan{};
      std::string plan_error;
      if (!resolve_vicreg_network_analytics_plan_(model(), &plan,
                                                  &plan_error)) {
        log_warn("[tsi.vicreg] debug network analytics skipped: %s\n",
                 plan_error.c_str());
        return RuntimeEventAction{};
      }
      const auto analytics =
          cuwacunu::piaabo::torch_compat::summarize_module_network_analytics(
              model(), plan.options);
      std::ostringstream oss;
      oss << "[tsi.vicreg][network_analytics] epoch_end"
          << " network=" << plan.network_label << "\n";
      oss << cuwacunu::piaabo::torch_compat::network_analytics_to_pretty_text(
          analytics, plan.network_label, /*use_color=*/true);
      log_dbg("%s", oss.str().c_str());
      return RuntimeEventAction{};
    }
    case RuntimeEventKind::EpochStart: {
      // Keep training counters/state across epochs and only commit any
      // leftover accumulation tail before the next epoch starts.
      if (!train_)
        return RuntimeEventAction{};
      (void)flush_pending_training_step_if_any_();
      if (epoch_optimizer_steps_ > 0) {
        // Safety flush in case an epoch boundary was missed upstream.
        apply_epoch_training_policy_();
      }
      return RuntimeEventAction{};
    }
    default:
      break;
    }
    return RuntimeEventAction{};
  }
