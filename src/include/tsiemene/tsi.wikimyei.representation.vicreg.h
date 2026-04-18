// ./include/tsiemene/tsi.wikimyei.representation.vicreg.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/network_design/network_design.h"
#include "tsiemene/tsi.cargo.validation.h"
#include "tsiemene/tsi.wikimyei.representation.h"
#include "wikimyei/evaluation/embedding_sequence_analytics/embedding_sequence_analytics.h"
#include "wikimyei/evaluation/transfer_evaluation_matrix/transfer_matrix_evaluation.h"

// Report-fragment metadata helpers:
#include "hero/hashimyei_hero/hashimyei_driver.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "iitepi/runtime_binding/runtime_binding.h"
#include "piaabo/latent_lineage_state/report_taxonomy.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"
#include "piaabo/torch_compat/data_analytics.h"
#include "piaabo/torch_compat/network_analytics.h"

// Your existing VICReg implementation:
#include "wikimyei/representation/VICReg/vicreg_4d.h"

namespace tsiemene {

using VicregTransferMatrixEvaluator =
    cuwacunu::wikimyei::evaluation::VicregTransferMatrixEvaluator;
using VicregEmbeddingSequenceEvaluator =
    cuwacunu::wikimyei::evaluation::VicregEmbeddingSequenceEvaluator;

struct vicreg_public_docking_shape_t {
  int C{0};
  int T{0};
  int D{0};
  int encoding_dims{0};
};

[[nodiscard]] inline vicreg_public_docking_shape_t
capture_vicreg_public_docking_shape_(
    const cuwacunu::wikimyei::vicreg_4d::VICReg_4D &model) {
  vicreg_public_docking_shape_t out{};
  out.C = model.C;
  out.T = model.T;
  out.D = model.D;
  out.encoding_dims = model.encoding_dims;
  return out;
}

struct vicreg_train_summary_snapshot_t {
  std::uint64_t epoch_index{0};
  std::uint64_t optimizer_steps{0};
  std::uint64_t trained_epochs{0};
  std::uint64_t trained_samples{0};
  std::uint64_t skipped_batches{0};
  double epoch_loss_mean{0.0};
  double epoch_inv_mean{0.0};
  double epoch_var_mean{0.0};
  double epoch_cov_raw_mean{0.0};
  double epoch_cov_weighted_mean{0.0};
  double last_committed_loss_mean{0.0};
  double learning_rate{0.0};
};

struct wikimyei_representation_vicreg_init_record_t : TsiWikimyeiInitRecord {
  bool enable_embedding_sequence_analytics_sidecar{true};
  bool has_embedding_sequence_analytics{false};
  cuwacunu::piaabo::torch_compat::data_analytics_options_t
      embedding_sequence_analytics_options{};
  cuwacunu::piaabo::torch_compat::sequence_analytics_report_t
      embedding_sequence_analytics_report{};
  cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_report_t
      embedding_sequence_symbolic_analytics_report{};
  std::filesystem::path embedding_sequence_analytics_file{};
  std::filesystem::path embedding_sequence_symbolic_analytics_file{};
  bool has_train_summary{false};
  vicreg_train_summary_snapshot_t train_summary_snapshot{};
  std::filesystem::path train_summary_file{};
};

using wikimyei_representation_vicreg_init_entry_t = TsiWikimyeiInitEntry;

[[nodiscard]] inline bool has_non_ws_ascii_(std::string_view text) {
  for (const unsigned char c : text) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

[[nodiscard]] inline std::string
effective_runtime_report_run_id_(std::string_view run_id,
                                 std::string_view hashimyei) {
  if (!run_id.empty())
    return std::string(run_id);
  if (hashimyei.empty())
    return "run.vicreg.unknown";
  return "run.vicreg." + std::string(hashimyei);
}

[[nodiscard]] inline std::string
sanitize_runtime_history_token_(std::string_view token) {
  std::string out;
  out.reserve(token.size());
  for (const unsigned char c : token) {
    const bool ok = (std::isalnum(c) != 0) || c == '_' || c == '-' || c == '.';
    out.push_back(ok ? static_cast<char>(c) : '_');
  }
  if (out.empty())
    return "run.unknown";
  return out;
}

[[nodiscard]] inline std::filesystem::path
runtime_history_file_path_(const std::filesystem::path &stable_file,
                           std::string_view history_token) {
  const std::string sanitized = sanitize_runtime_history_token_(history_token);
  const std::string stem = stable_file.stem().string();
  std::string stem_prefix = stem;
  constexpr std::string_view kLatestSuffix = ".latest";
  if (stem_prefix.size() > kLatestSuffix.size() &&
      stem_prefix.compare(stem_prefix.size() - kLatestSuffix.size(),
                          kLatestSuffix.size(), kLatestSuffix) == 0) {
    stem_prefix.erase(stem_prefix.size() - kLatestSuffix.size());
  }
  return stable_file.parent_path() /
         (stem_prefix + "." + sanitized + stable_file.extension().string());
}

inline void append_runtime_string_entry_(runtime_lls_document_t *document,
                                         std::string_view key,
                                         std::string_view value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value)));
}

inline void
append_runtime_string_entry_if_nonempty_(runtime_lls_document_t *document,
                                         std::string_view key,
                                         std::string_view value) {
  if (!document || value.empty())
    return;
  append_runtime_string_entry_(document, key, value);
}

inline void append_runtime_int_entry_(runtime_lls_document_t *document,
                                      std::string_view key,
                                      std::int64_t value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry(
          std::string(key), value));
}

inline void append_runtime_u64_entry_(runtime_lls_document_t *document,
                                      std::string_view key,
                                      std::uint64_t value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value, "(0,+inf)"));
}

inline void append_runtime_bool_entry_(runtime_lls_document_t *document,
                                       std::string_view key, bool value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_bool_entry(
          std::string(key), value));
}

inline void append_runtime_double_entry_(runtime_lls_document_t *document,
                                         std::string_view key, double value) {
  if (!document || !std::isfinite(value))
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          std::string(key), value));
}

struct vicreg_network_analytics_plan_t {
  cuwacunu::piaabo::torch_compat::network_analytics_options_t options{};
  std::string network_label{};
};

[[nodiscard]] inline bool resolve_vicreg_network_analytics_plan_(
    const cuwacunu::wikimyei::vicreg_4d::VICReg_4D &model,
    vicreg_network_analytics_plan_t *out, std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "internal: null network analytics plan output";
    return false;
  }

  if (!has_non_ws_ascii_(model.network_design_grammar) ||
      !has_non_ws_ascii_(model.network_design_dsl)) {
    if (error) {
      *error = "missing network_design payload (expected explicit "
               "NETWORK_ANALYTICS_POLICY node)";
    }
    return false;
  }

  try {
    const auto design_ir =
        cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
            model.network_design_grammar, model.network_design_dsl);
    std::string options_error;
    if (!cuwacunu::piaabo::torch_compat::
            resolve_network_analytics_options_from_network_design(
                design_ir, &out->options, &options_error)) {
      if (error) {
        *error = options_error.empty()
                     ? "unable to resolve network analytics options"
                     : options_error;
      }
      return false;
    }
    out->network_label = model.network_design_network_id;
    if (!has_non_ws_ascii_(out->network_label) &&
        has_non_ws_ascii_(design_ir.network_id)) {
      out->network_label = design_ir.network_id;
    }
    if (!has_non_ws_ascii_(out->network_label)) {
      out->network_label = model.component_name;
    }
    return true;
  } catch (const std::exception &e) {
    if (error) {
      *error = std::string("failed to decode network_design payload (") +
               e.what() + ")";
    }
    return false;
  }
}

[[nodiscard]] inline bool write_vicreg_network_analytics_sidecar_(
    const cuwacunu::wikimyei::vicreg_4d::VICReg_4D &model,
    const std::filesystem::path &weights_file, std::string_view canonical_path,
    std::string_view family_canonical_path, std::string_view report_fragment_id,
    std::string_view contract_hash, std::string_view binding_id,
    std::string_view run_id, std::string_view source_runtime_cursor,
    bool has_wave_cursor, std::uint64_t wave_cursor,
    std::filesystem::path *out_sidecar_file, std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out_sidecar_file) {
    if (error)
      *error = "internal: null network analytics sidecar output path";
    return false;
  }
  (void)canonical_path;
  (void)contract_hash;
  (void)run_id;
  vicreg_network_analytics_plan_t plan{};
  std::string plan_error;
  if (!resolve_vicreg_network_analytics_plan_(model, &plan, &plan_error)) {
    if (error)
      *error = plan_error;
    return false;
  }
  auto network_report_identity =
      make_component_report_identity(std::string(family_canonical_path) + "." +
                                         std::string(report_fragment_id),
                                     binding_id,
                                     cuwacunu::piaabo::latent_lineage_state::
                                         report_taxonomy::kEmbeddingNetwork);
  network_report_identity.source_runtime_cursor =
      std::string(source_runtime_cursor);
  network_report_identity.has_wave_cursor = has_wave_cursor;
  network_report_identity.wave_cursor = wave_cursor;
  return cuwacunu::piaabo::torch_compat::
      write_network_analytics_sidecar_for_checkpoint(
          model, weights_file, out_sidecar_file, plan.options, error,
          network_report_identity);
}

[[nodiscard]] inline bool load_wikimyei_representation_vicreg_init_into_model(
    std::string_view hashimyei, const std::string &contract_hash,
    const std::string &component_name, torch::Device preferred_device,
    vicreg_public_docking_shape_t expected_public_docking,
    std::unique_ptr<cuwacunu::wikimyei::vicreg_4d::VICReg_4D> *model_slot,
    std::string *error = nullptr);

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t
update_wikimyei_representation_vicreg_init(
    std::string hashimyei,
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D *model = nullptr,
    bool enable_network_analytics_sidecar = true,
    bool enable_embedding_sequence_analytics_sidecar = true,
    std::string contract_hash = {}, std::string binding_id = {},
    std::string run_id = {}, std::string source_runtime_cursor = {},
    bool has_wave_cursor = false, std::uint64_t wave_cursor = 0,
    const cuwacunu::piaabo::torch_compat::sequence_analytics_report_t
        *embedding_sequence_analytics_report = nullptr,
    const cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_report_t
        *embedding_sequence_symbolic_analytics_report = nullptr,
    const vicreg_train_summary_snapshot_t *train_summary_snapshot = nullptr,
    cuwacunu::piaabo::torch_compat::data_analytics_options_t
        embedding_sequence_analytics_options = {});

class TsiWikimyeiRepresentationVicreg final : public TsiWikimyeiRepresentation {
public:
  static constexpr DirectiveId IN_STEP = directive_id::Step;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_LOSS = directive_id::Loss;
  static constexpr DirectiveId OUT_META = directive_id::Meta;

  TsiWikimyeiRepresentationVicreg(TsiId id, std::string instance_name,
                                  std::string contract_hash,
                                  std::string representation_hashimyei,
                                  std::string component_name, int C, int T,
                                  int D, bool train = false,
                                  bool use_swa = true,
                                  bool detach_to_cpu = true)
      : id_(id), instance_name_(std::move(instance_name)),
        contract_hash_(std::move(contract_hash)),
        representation_hashimyei_(std::move(representation_hashimyei)),
        component_name_(std::move(component_name)),
        model_(std::make_unique<cuwacunu::wikimyei::vicreg_4d::VICReg_4D>(
            contract_hash_, component_name_, C, T, D)) {
    apply_runtime_policy_from_jkimyei_(train, use_swa, detach_to_cpu);
    maybe_initialize_embedding_sequence_evaluator_();
    maybe_initialize_transfer_matrix_evaluator_();
  }

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.wikimyei.representation.vicreg";
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
    const bool loaded = load_wikimyei_representation_vicreg_init_into_model(
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
    if (runtime_context) {
      const auto *io_ctx =
          static_cast<const TsiWikimyeiRuntimeIoContext *>(runtime_context);
      enable_network_analytics_sidecar = io_ctx->enable_debug_outputs;
      enable_embedding_sequence_analytics_sidecar =
          io_ctx->enable_debug_outputs;
      if (!io_ctx->binding_id.empty()) {
        runtime_binding_id_ = io_ctx->binding_id;
      }
      if (!io_ctx->run_id.empty()) {
        runtime_run_id_ = io_ctx->run_id;
      }
      if (!io_ctx->source_runtime_cursor.empty()) {
        runtime_source_runtime_cursor_ = io_ctx->source_runtime_cursor;
      }
      if (io_ctx->has_wave_cursor) {
        runtime_has_wave_cursor_ = true;
        runtime_wave_cursor_ = io_ctx->wave_cursor;
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
    auto out = update_wikimyei_representation_vicreg_init(
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
            : cuwacunu::piaabo::torch_compat::data_analytics_options_t{});
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
    return "tsi.wikimyei.representation.vicreg.init.v1";
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
      log_info("%s", oss.str().c_str());
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

private:
  [[nodiscard]] static std::string build_transfer_matrix_report_canonical_path_(
      std::string_view representation_hashimyei) {
    std::string out("tsi.wikimyei.representation.vicreg");
    if (!has_non_ws_ascii_(representation_hashimyei))
      return out;
    out.push_back('.');
    out.append(representation_hashimyei);
    return out;
  }

  [[nodiscard]] std::filesystem::path
  build_transfer_matrix_runtime_report_path_(
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
    if (!has_non_ws_ascii_(report_lls) &&
        !has_non_ws_ascii_(matrix_report_lls)) {
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
        .epoch_cov_weighted_mean = epoch_cov_weighted_sum_ /
                                   static_cast<double>(epoch_optimizer_steps_),
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
         .last_committed_loss_mean =
             last_train_summary_.last_committed_loss_mean,
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

  [[nodiscard]] cuwacunu::wikimyei::vicreg_4d::VICReg_4D &model() noexcept {
    return *model_;
  }
  [[nodiscard]] const cuwacunu::wikimyei::vicreg_4d::VICReg_4D &
  model() const noexcept {
    return *model_;
  }

  std::unique_ptr<cuwacunu::wikimyei::vicreg_4d::VICReg_4D> model_;
};

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
wikimyei_representation_vicreg_store_root() {
  return cuwacunu::hashimyei::canonical_path_directory(
      cuwacunu::hashimyei::store_root(), "tsi.wikimyei.representation.vicreg");
}

[[nodiscard]] inline bool
is_valid_wikimyei_representation_vicreg_hash(std::string_view hashimyei) {
  std::uint64_t parsed = 0;
  return normalize_wikimyei_hex_hash(hashimyei, nullptr, &parsed);
}

[[nodiscard]] inline bool resolve_wikimyei_representation_vicreg_directory(
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

  const fs::path store_root = wikimyei_representation_vicreg_store_root();
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
list_wikimyei_representation_vicreg_report_fragments() {
  return cuwacunu::hashimyei::discover_created_report_fragments_for(
      "representation", "vicreg");
}

[[nodiscard]] inline std::vector<wikimyei_representation_vicreg_init_entry_t>
list_wikimyei_representation_vicreg_init_entries() {
  const auto report_fragments =
      list_wikimyei_representation_vicreg_report_fragments();
  std::vector<wikimyei_representation_vicreg_init_entry_t> out;
  out.reserve(report_fragments.size());
  for (const auto &item : report_fragments) {
    wikimyei_representation_vicreg_init_entry_t e{};
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
      "schema=tsi.wikimyei.representation.vicreg.weights.init.v1\n"
      "state=placeholder\n";
  return write_wikimyei_text_file(weights_file, payload, error);
}

[[nodiscard]] inline std::string next_wikimyei_representation_vicreg_hash(
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

inline constexpr std::string_view kWikimyeiVicregCanonicalType =
    "tsi.wikimyei.representation.vicreg";
inline constexpr std::string_view kWikimyeiVicregFamily = "representation";
inline constexpr std::string_view kWikimyeiVicregModel = "vicreg";

[[nodiscard]] inline bool
save_wikimyei_representation_vicreg_report_fragment_with_driver(
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
  if (!resolve_wikimyei_representation_vicreg_directory(
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
  auto *out = static_cast<wikimyei_representation_vicreg_init_record_t *>(
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
      std::string(kWikimyeiVicregCanonicalType) + "." + normalized_hashimyei;
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
    // cuwacunu::wikimyei::vicreg_4d::VICReg_4D.
    auto *model = static_cast<cuwacunu::wikimyei::vicreg_4d::VICReg_4D *>(
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
              kWikimyeiVicregCanonicalType, normalized_hashimyei, contract_hash,
              binding_id, effective_run_id,
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
          ? "tsi.wikimyei.representation.vicreg.init()"
          : action.canonical_action;

  std::ostringstream metadata;
  metadata << "schema=tsi.wikimyei.representation.vicreg.init.v1\n";
  metadata << "canonical_action=" << canonical_action << "\n";
  metadata << "canonical_target=tsi.wikimyei.representation.vicreg\n";
  metadata << "family=" << kWikimyeiVicregFamily << "\n";
  metadata << "model=" << kWikimyeiVicregModel << "\n";
  metadata << "hashimyei=" << normalized_hashimyei << "\n";
  metadata << "canonical_path=" << canonical_path << "\n";
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
          static_cast<const cuwacunu::wikimyei::vicreg_4d::VICReg_4D *>(
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
          "tsi.wikimyei.representation.vicreg.train_summary.v1");
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
      if (action.object_handle) {
        const auto *model =
            static_cast<const cuwacunu::wikimyei::vicreg_4d::VICReg_4D *>(
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
        &status_lls, "schema", "tsi.wikimyei.representation.vicreg.status.v1");
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
  manifest.family_canonical_path = std::string(kWikimyeiVicregCanonicalType);
  manifest.family = std::string(kWikimyeiVicregFamily);
  manifest.model = std::string(kWikimyeiVicregModel);
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
load_wikimyei_representation_vicreg_report_fragment_with_driver(
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
  if (!resolve_wikimyei_representation_vicreg_directory(
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
        std::string(kWikimyeiVicregCanonicalType)) {
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
    // cuwacunu::wikimyei::vicreg_4d::VICReg_4D.
    auto *model = static_cast<cuwacunu::wikimyei::vicreg_4d::VICReg_4D *>(
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
ensure_wikimyei_representation_vicreg_driver_registered(
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (cuwacunu::hashimyei::has_report_fragment_driver(
          kWikimyeiVicregCanonicalType))
    return true;

  cuwacunu::hashimyei::report_fragment_driver_t driver{};
  driver.family_canonical_path = std::string(kWikimyeiVicregCanonicalType);
  driver.family = std::string(kWikimyeiVicregFamily);
  driver.model = std::string(kWikimyeiVicregModel);
  driver.save = save_wikimyei_representation_vicreg_report_fragment_with_driver;
  driver.load = load_wikimyei_representation_vicreg_report_fragment_with_driver;

  if (cuwacunu::hashimyei::register_report_fragment_driver(std::move(driver),
                                                           error))
    return true;
  // Registration may race in multi-entry contexts; treat "already registered"
  // as success.
  return cuwacunu::hashimyei::has_report_fragment_driver(
      kWikimyeiVicregCanonicalType);
}

[[nodiscard]] inline bool
write_wikimyei_representation_vicreg_report_fragment_payload(
    std::string canonical_action,
    wikimyei_representation_vicreg_init_record_t *out,
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D *model = nullptr) {
  if (!out)
    return false;

  out->metadata_encrypted = false;
  out->metadata_plaintext_fallback = false;
  out->metadata_warning.clear();
  out->canonical_path.clear();
  out->weights_file.clear();
  out->train_summary_file.clear();

  std::string registration_error;
  if (!ensure_wikimyei_representation_vicreg_driver_registered(
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
  if (!resolve_wikimyei_representation_vicreg_directory(
          normalized_hashimyei, false, &report_fragment_directory,
          &out->error)) {
    return false;
  }
  out->hashimyei = normalized_hashimyei;
  out->report_fragment_directory = report_fragment_directory;
  action.family_canonical_path = std::string(kWikimyeiVicregCanonicalType);
  action.family = std::string(kWikimyeiVicregFamily);
  action.model = std::string(kWikimyeiVicregModel);
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

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t
persist_wikimyei_representation_vicreg_init(
    cuwacunu::wikimyei::vicreg_4d::VICReg_4D *model = nullptr) {
  namespace fs = std::filesystem;
  wikimyei_representation_vicreg_init_record_t out{};

  out.store_root = wikimyei_representation_vicreg_store_root();

  std::error_code ec;
  fs::create_directories(out.store_root, ec);
  if (ec) {
    out.error = "cannot create wikimyei report_fragment root: " +
                out.store_root.string();
    return out;
  }

  out.hashimyei = next_wikimyei_representation_vicreg_hash(out.store_root);
  out.report_fragment_directory = out.store_root / out.hashimyei;

  fs::create_directories(out.report_fragment_directory, ec);
  if (ec) {
    out.error = "cannot create wikimyei report_fragment directory: " +
                out.report_fragment_directory.string();
    return out;
  }

  if (!write_wikimyei_representation_vicreg_report_fragment_payload(
          "tsi.wikimyei.representation.vicreg.init()", &out, model)) {
    return out;
  }

  return out;
}

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t
update_wikimyei_representation_vicreg_init(
    std::string hashimyei, cuwacunu::wikimyei::vicreg_4d::VICReg_4D *model,
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
        embedding_sequence_analytics_options) {
  namespace fs = std::filesystem;
  wikimyei_representation_vicreg_init_record_t out{};
  out.store_root = wikimyei_representation_vicreg_store_root();
  out.enable_network_analytics_sidecar = enable_network_analytics_sidecar;
  out.enable_embedding_sequence_analytics_sidecar =
      enable_embedding_sequence_analytics_sidecar;
  out.contract_hash = std::move(contract_hash);
  out.binding_id = std::move(binding_id);
  out.run_id = std::move(run_id);
  out.source_runtime_cursor = std::move(source_runtime_cursor);
  out.has_wave_cursor = has_wave_cursor;
  out.wave_cursor = wave_cursor;
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
  if (!resolve_wikimyei_representation_vicreg_directory(
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

  if (!write_wikimyei_representation_vicreg_report_fragment_payload(
          "tsi.wikimyei.representation.vicreg.edit()", &out, model)) {
    return out;
  }

  return out;
}

[[nodiscard]] inline bool load_wikimyei_representation_vicreg_init_into_model(
    std::string_view hashimyei, const std::string &contract_hash,
    const std::string &component_name, torch::Device preferred_device,
    vicreg_public_docking_shape_t expected_public_docking,
    std::unique_ptr<cuwacunu::wikimyei::vicreg_4d::VICReg_4D> *model_slot,
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
  if (!resolve_wikimyei_representation_vicreg_directory(
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
        std::string(kWikimyeiVicregCanonicalType)) {
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
      [&](const cuwacunu::wikimyei::vicreg_4d::VICReg_4D &candidate) -> bool {
    if (candidate.C != expected_public_docking.C ||
        candidate.T != expected_public_docking.T ||
        candidate.D != expected_public_docking.D ||
        candidate.encoding_dims != expected_public_docking.encoding_dims) {
      if (error) {
        std::ostringstream oss;
        oss << "vicreg public docking mismatch: expected C/T/D/E=("
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

  std::unique_ptr<cuwacunu::wikimyei::vicreg_4d::VICReg_4D> replacement{};
  try {
    replacement = std::make_unique<cuwacunu::wikimyei::vicreg_4d::VICReg_4D>(
        contract_hash, component_name, weights_file.string(), preferred_device);
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

[[nodiscard]] inline bool delete_wikimyei_representation_vicreg_init(
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
  if (!resolve_wikimyei_representation_vicreg_directory(normalized_hashimyei,
                                                        true, &target, error)) {
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

[[nodiscard]] inline wikimyei_representation_vicreg_init_record_t
invoke_wikimyei_representation_vicreg_init_from_config() {
  return persist_wikimyei_representation_vicreg_init();
}

} // namespace tsiemene
