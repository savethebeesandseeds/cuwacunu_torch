// ./include/tsiemene/tsi.wikimyei.inference.mdn.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "hero/hashimyei_hero/hashimyei_driver.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "jkimyei/training_setup/jk_setup.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"
#include "tsiemene/tsi.cargo.validation.h"
#include "tsiemene/tsi.report.h"
#include "tsiemene/tsi.wikimyei.inference.h"
#include "wikimyei/inference/expected_value/expected_value.h"

namespace tsiemene {

struct wikimyei_inference_mdn_init_record_t : TsiWikimyeiInitRecord {
  std::filesystem::path status_file{};
  std::filesystem::path network_analytics_file{};
  double last_committed_loss_mean{0.0};
  int epoch_optimizer_steps{0};
};

inline constexpr std::string_view kWikimyeiMdnCanonicalType =
    "tsi.wikimyei.inference.mdn";
inline constexpr std::string_view kWikimyeiMdnFamily = "inference";
inline constexpr std::string_view kWikimyeiMdnModel = "mdn";

[[nodiscard]] inline bool load_wikimyei_inference_mdn_init_into_model(
    std::string_view hashimyei, const std::string &contract_hash,
    const std::string &component_name,
    std::unique_ptr<cuwacunu::wikimyei::ExpectedValue> *model_slot,
    std::string *error = nullptr);

[[nodiscard]] inline wikimyei_inference_mdn_init_record_t
update_wikimyei_inference_mdn_init(
    std::string hashimyei, cuwacunu::wikimyei::ExpectedValue *model,
    std::string contract_hash = {}, std::string binding_id = {},
    std::string run_id = {}, std::string source_runtime_cursor = {},
    bool has_wave_cursor = false, std::uint64_t wave_cursor = 0,
    double last_committed_loss_mean = 0.0, int epoch_optimizer_steps = 0,
    bool enable_network_analytics_sidecar = false);

class TsiWikimyeiInferenceMdn final : public TsiWikimyeiInference {
public:
  static constexpr DirectiveId IN_STEP = directive_id::Step;
  static constexpr DirectiveId OUT_PAYLOAD = directive_id::Payload;
  static constexpr DirectiveId OUT_FUTURE = directive_id::Future;
  static constexpr DirectiveId OUT_LOSS = directive_id::Loss;
  static constexpr DirectiveId OUT_META = directive_id::Meta;

  TsiWikimyeiInferenceMdn(TsiId id, std::string instance_name,
                          std::string contract_hash, std::string hashimyei,
                          std::string component_name, bool train = false)
      : id_(id), instance_name_(std::move(instance_name)),
        contract_hash_(std::move(contract_hash)),
        hashimyei_(std::move(hashimyei)),
        component_name_(std::move(component_name)),
        model_(std::make_unique<cuwacunu::wikimyei::ExpectedValue>(
            contract_hash_, component_name_)),
        train_(train) {}

  [[nodiscard]] std::string_view type_name() const noexcept override {
    return "tsi.wikimyei.inference.mdn";
  }
  [[nodiscard]] std::string_view instance_name() const noexcept override {
    return instance_name_;
  }
  [[nodiscard]] TsiId id() const noexcept override { return id_; }
  [[nodiscard]] bool emits_expectation_directive() const noexcept override {
    return true;
  }
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
    if (runtime_context) {
      capture_runtime_io_context_(
          *static_cast<const TsiWikimyeiRuntimeIoContext *>(runtime_context));
    }
    const bool loaded = load_wikimyei_inference_mdn_init_into_model(
        hashimyei, contract_hash_, component_name_, &model_, error);
    if (loaded) {
      reset_epoch_training_accumulators_();
      last_committed_loss_mean_ =
          model().runtime_state.last_committed_loss_mean;
    }
    return loaded;
  }
  [[nodiscard]] bool
  runtime_save_to_hashimyei(std::string_view hashimyei,
                            std::string *error = nullptr,
                            const void *runtime_context = nullptr) override {
    if (runtime_context) {
      capture_runtime_io_context_(
          *static_cast<const TsiWikimyeiRuntimeIoContext *>(runtime_context));
    }
    if (model().runtime_state.has_pending_grad) {
      try {
        const bool committed = model().finalize_pending_training_step();
        if (committed) {
          last_committed_loss_mean_ =
              model().runtime_state.last_committed_loss_mean;
          epoch_loss_sum_ += last_committed_loss_mean_;
          ++epoch_optimizer_steps_;
        }
      } catch (const std::exception &e) {
        if (error)
          *error = std::string("failed to flush ExpectedValue before save: ") +
                   e.what();
        return false;
      } catch (...) {
        if (error)
          *error =
              "failed to flush ExpectedValue before save: unknown exception";
        return false;
      }
    }
    const auto result = update_wikimyei_inference_mdn_init(
        std::string(hashimyei), model_.get(), contract_hash_,
        runtime_binding_id_, runtime_run_id_, runtime_source_runtime_cursor_,
        runtime_has_wave_cursor_, runtime_wave_cursor_,
        last_committed_loss_mean_, epoch_optimizer_steps_,
        runtime_enable_debug_outputs_);
    if (!result.ok) {
      if (error)
        *error = result.error;
      return false;
    }
    if (error)
      error->clear();
    return true;
  }
  [[nodiscard]] std::string_view
  init_report_fragment_schema() const noexcept override {
    return "tsi.wikimyei.inference.mdn.init.v1";
  }
  [[nodiscard]] std::string_view
  report_fragment_family() const noexcept override {
    return kWikimyeiMdnFamily;
  }
  [[nodiscard]] std::string_view
  report_fragment_model() const noexcept override {
    return kWikimyeiMdnModel;
  }

  [[nodiscard]] std::span<const DirectiveSpec>
  directives() const noexcept override {
    static constexpr DirectiveSpec kDirectives[] = {
        directive(IN_STEP, DirectiveDir::In, KindSpec::Cargo(),
                  "representation cargo with encoding/future target ingress"),
        directive(OUT_PAYLOAD, DirectiveDir::Out, KindSpec::Cargo(),
                  "observation cargo forwarded after expected-value inference"),
        directive(OUT_FUTURE, DirectiveDir::Out, KindSpec::Tensor(),
                  "conditional expectation E[Y|X] tensor"),
        directive(OUT_LOSS, DirectiveDir::Out, KindSpec::Tensor(),
                  "MDN negative-log-likelihood training loss"),
        directive(OUT_META, DirectiveDir::Out, KindSpec::String(),
                  "runtime trace/meta stream"),
    };
    return std::span<const DirectiveSpec>(kDirectives, 5);
  }

  void set_train(bool on) noexcept { train_ = on; }

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
    out->supports_runtime_profile_switch = false;
    out->optimizer_steps = model().runtime_state.optimizer_steps;
    out->accumulate_steps = model().training_policy.accumulate_steps;
    out->accum_counter = model().runtime_state.accum_counter;
    out->trained_epochs = static_cast<std::uint64_t>(
        std::max<std::int64_t>(0, model().total_epochs_trained_));
    out->trained_samples = model().runtime_state.trained_samples;
    out->skipped_batches = model().runtime_state.skipped_batches;
    out->has_pending_grad = model().runtime_state.has_pending_grad;
    out->skip_on_nan = model().training_policy.skip_on_nan;
    out->zero_grad_set_to_none = model().training_policy.zero_grad_set_to_none;
    out->clip_norm = model().training_policy.clip_norm;
    out->clip_value = model().training_policy.clip_value;
    out->last_committed_loss_mean =
        model().runtime_state.last_committed_loss_mean;

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
    const bool had_pending = model().runtime_state.has_pending_grad;
    bool optimizer_step_applied = false;
    try {
      optimizer_step_applied = model().finalize_pending_training_step();
    } catch (const std::exception &e) {
      if (error)
        *error = std::string("failed to flush ExpectedValue training step: ") +
                 e.what();
      return false;
    } catch (...) {
      if (error)
        *error =
            "failed to flush ExpectedValue training step: unknown exception";
      return false;
    }
    if (optimizer_step_applied) {
      last_committed_loss_mean_ =
          model().runtime_state.last_committed_loss_mean;
      epoch_loss_sum_ += last_committed_loss_mean_;
      ++epoch_optimizer_steps_;
    }
    if (out) {
      *out = TsiWikimyeiJkimyeiFlushResult{};
      out->had_pending_grad = had_pending;
      out->optimizer_step_applied = optimizer_step_applied;
      out->has_pending_grad_after = model().runtime_state.has_pending_grad;
      out->last_committed_loss_mean =
          model().runtime_state.last_committed_loss_mean;
    }
    return true;
  }

  void step(const Wave &wave, Ingress in, RuntimeContext &ctx,
            Emitter &out) override {
    if (in.directive != IN_STEP)
      return;
    if (in.signal.kind != PayloadKind::Cargo)
      return;
    capture_runtime_context_(&wave, ctx);

    std::string cargo_error{};
    const CargoValidationStage validation_stage =
        train_ ? CargoValidationStage::ExpectedValueTrainIn
               : CargoValidationStage::ExpectedValueIn;
    if (!validate_observation_cargo(in.signal.cargo, validation_stage,
                                    &cargo_error)) {
      emit_meta_(wave, out,
                 std::string("cargo.invalid stage=") +
                     std::string(cargo_stage_token(validation_stage)) +
                     " reason=" + cargo_error);
      return;
    }

    const auto &sample = *in.signal.cargo;
    emit_in_trace_(wave, out, sample);

    try {
      auto expectation = predict_expectation_(sample).detach().to(torch::kCPU);
      const std::string expectation_shape = tensor_shape_(expectation);
      out.emit_tensor(wave, OUT_FUTURE, std::move(expectation));
      emit_meta_(wave, out,
                 "expected_value.out expectation=" + expectation_shape);
    } catch (const std::exception &e) {
      emit_meta_(wave, out,
                 std::string("expected_value.out expectation=failed reason=") +
                     e.what());
      return;
    } catch (...) {
      emit_meta_(wave, out,
                 "expected_value.out expectation=failed reason=unknown");
      return;
    }

    using sample_t = cuwacunu::camahjucunu::data::observation_sample_t;
    auto out_sample = std::make_shared<sample_t>(sample);
    out.emit_cargo(wave, OUT_PAYLOAD, std::move(out_sample));

    if (!train_)
      return;
    try {
      const auto step = train_one_batch_(sample);
      if (step.skipped || !step.loss.defined()) {
        emit_meta_(wave, out, "expected_value.out loss=skipped");
        return;
      }
      auto detached_loss = step.loss.detach().to(torch::kCPU);
      out.emit_tensor(wave, OUT_LOSS, std::move(detached_loss));
      emit_meta_(wave, out,
                 "expected_value.out loss=" + tensor_shape_(step.loss) +
                     (step.optimizer_step_applied ? " optimizer_step=applied"
                                                  : " optimizer_step=pending"));
    } catch (const std::exception &e) {
      emit_meta_(wave, out,
                 std::string("expected_value.train failed reason=") + e.what());
    } catch (...) {
      emit_meta_(wave, out, "expected_value.train failed reason=unknown");
    }
  }

  RuntimeEventAction on_event(const RuntimeEvent &event, RuntimeContext &ctx,
                              Emitter &out) override {
    capture_runtime_context_(event.wave, ctx);
    switch (event.kind) {
    case RuntimeEventKind::RunStart:
      reset_epoch_training_accumulators_();
      return RuntimeEventAction{};
    case RuntimeEventKind::EpochEnd:
      if (train_)
        apply_epoch_training_policy_();
      return RuntimeEventAction{};
    case RuntimeEventKind::EpochStart:
      if (train_ && epoch_optimizer_steps_ > 0) {
        apply_epoch_training_policy_();
      }
      return RuntimeEventAction{};
    default:
      break;
    }
    return RuntimeEventAction{};
  }

private:
  [[nodiscard]] torch::Tensor
  predict_expectation_(const ObservationCargo &sample) {
    torch::NoGradGuard ng;
    model().semantic_model->eval();
    auto enc = sample.encoding.to(model().semantic_model->device,
                                  model().semantic_model->dtype,
                                  /*non_blocking=*/true,
                                  /*copy=*/false);
    auto mask =
        sample.mask.defined()
            ? sample.mask.to(model().semantic_model->device, torch::kFloat32,
                             /*non_blocking=*/true, /*copy=*/false)
            : torch::Tensor();
    return model().predict_expected_value_from_encoding(enc, mask);
  }

  [[nodiscard]] cuwacunu::wikimyei::ExpectedValue::train_step_result_t
  train_one_batch_(const ObservationCargo &sample) {
    auto result =
        model().train_one_batch(sample.encoding, sample.future_features,
                                sample.future_mask, sample.mask);
    if (result.optimizer_step_applied) {
      last_committed_loss_mean_ =
          model().runtime_state.last_committed_loss_mean;
      epoch_loss_sum_ += last_committed_loss_mean_;
      ++epoch_optimizer_steps_;
    }
    return result;
  }

  void apply_epoch_training_policy_() {
    if (model().runtime_state.has_pending_grad) {
      const bool committed = model().finalize_pending_training_step();
      if (committed) {
        last_committed_loss_mean_ =
            model().runtime_state.last_committed_loss_mean;
        epoch_loss_sum_ += last_committed_loss_mean_;
        ++epoch_optimizer_steps_;
      }
    }
    if (epoch_optimizer_steps_ <= 0)
      return;
    const double metric =
        epoch_loss_sum_ / static_cast<double>(epoch_optimizer_steps_);
    if (std::isfinite(metric) && metric < model().best_metric_) {
      model().best_metric_ = metric;
      model().best_epoch_ = static_cast<int>(model().total_epochs_trained_);
    }
    if (model().lr_sched) {
      const auto mode = model().lr_sched->mode;
      if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch) {
        model().lr_sched->step();
        ++model().scheduler_epoch_steps_;
      } else if (mode ==
                 cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpochWithMetric) {
        model().lr_sched->step(metric);
        ++model().scheduler_epoch_steps_;
      }
    }
    ++model().total_epochs_trained_;
    reset_epoch_training_accumulators_();
  }

  void reset_epoch_training_accumulators_() {
    epoch_loss_sum_ = 0.0;
    epoch_optimizer_steps_ = 0;
  }

  void capture_runtime_context_(const Wave *wave, const RuntimeContext &ctx) {
    runtime_enable_debug_outputs_ = ctx.debug_enabled;
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
      return;
    }
    if (!wave)
      return;
    std::uint64_t packed_wave_cursor = 0;
    if (cuwacunu::piaabo::latent_lineage_state::pack_runtime_wave_cursor(
            wave->cursor.i, wave->cursor.episode, wave->cursor.batch,
            &packed_wave_cursor)) {
      runtime_has_wave_cursor_ = true;
      runtime_wave_cursor_ = packed_wave_cursor;
    }
  }

  void capture_runtime_io_context_(const TsiWikimyeiRuntimeIoContext &io_ctx) {
    runtime_enable_debug_outputs_ = io_ctx.enable_debug_outputs;
    if (!io_ctx.binding_id.empty())
      runtime_binding_id_ = io_ctx.binding_id;
    if (!io_ctx.run_id.empty())
      runtime_run_id_ = io_ctx.run_id;
    if (!io_ctx.source_runtime_cursor.empty())
      runtime_source_runtime_cursor_ = io_ctx.source_runtime_cursor;
    if (io_ctx.has_wave_cursor) {
      runtime_has_wave_cursor_ = true;
      runtime_wave_cursor_ = io_ctx.wave_cursor;
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

  void emit_in_trace_(const Wave &wave, Emitter &out,
                      const ObservationCargo &sample) const {
    std::ostringstream oss;
    oss << "expected_value.in encoding=" << tensor_shape_(sample.encoding)
        << " future=" << tensor_shape_(sample.future_features)
        << " train=" << (train_ ? "true" : "false");
    emit_meta_(wave, out, oss.str());
  }

  void emit_meta_(const Wave &wave, Emitter &out, std::string msg) const {
    out.emit_string(wave, OUT_META, std::move(msg));
  }

  [[nodiscard]] cuwacunu::wikimyei::ExpectedValue &model() noexcept {
    return *model_;
  }
  [[nodiscard]] const cuwacunu::wikimyei::ExpectedValue &
  model() const noexcept {
    return *model_;
  }

  TsiId id_{};
  std::string instance_name_{};
  std::string contract_hash_{};
  std::string hashimyei_{};
  std::string component_name_{};
  std::string runtime_binding_id_{};
  std::string runtime_run_id_{};
  std::string runtime_source_runtime_cursor_{};
  bool runtime_has_wave_cursor_{false};
  std::uint64_t runtime_wave_cursor_{0};
  bool runtime_enable_debug_outputs_{false};
  bool train_{false};
  double epoch_loss_sum_{0.0};
  int epoch_optimizer_steps_{0};
  double last_committed_loss_mean_{0.0};
  std::unique_ptr<cuwacunu::wikimyei::ExpectedValue> model_{};
};

[[nodiscard]] inline bool
normalize_wikimyei_mdn_hex_hash(std::string_view hashimyei,
                                std::string *out_hashimyei,
                                std::uint64_t *out_ordinal = nullptr) {
  return cuwacunu::hashimyei::normalize_hex_hash_name(hashimyei, out_hashimyei,
                                                      out_ordinal);
}

[[nodiscard]] inline std::filesystem::path wikimyei_inference_mdn_store_root() {
  return cuwacunu::hashimyei::canonical_path_directory(
      cuwacunu::hashimyei::store_root(), kWikimyeiMdnCanonicalType);
}

[[nodiscard]] inline bool resolve_wikimyei_inference_mdn_directory(
    std::string_view hashimyei, bool require_existing,
    std::filesystem::path *out_directory, std::string *error = nullptr) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();
  if (!out_directory) {
    if (error)
      *error = "mdn report_fragment directory output pointer is null";
    return false;
  }

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_mdn_hex_hash(hashimyei, &normalized_hashimyei)) {
    if (error)
      *error = "invalid mdn hashimyei id: " + std::string(hashimyei);
    return false;
  }

  const fs::path canonical_directory =
      wikimyei_inference_mdn_store_root() / normalized_hashimyei;
  std::error_code ec;
  if (fs::exists(canonical_directory, ec)) {
    if (!fs::is_directory(canonical_directory, ec)) {
      if (error) {
        *error = "mdn report_fragment path is not a directory: " +
                 canonical_directory.string();
      }
      return false;
    }
    *out_directory = canonical_directory;
    return true;
  }
  if (ec) {
    if (error) {
      *error = "cannot inspect mdn report_fragment directory: " +
               canonical_directory.string();
    }
    return false;
  }
  if (require_existing) {
    if (error) {
      *error = "mdn report_fragment not found: " + canonical_directory.string();
    }
    return false;
  }
  *out_directory = canonical_directory;
  return true;
}

[[nodiscard]] inline bool
write_wikimyei_mdn_text_file(const std::filesystem::path &target,
                             std::string_view content,
                             std::string *error = nullptr) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();

  std::error_code ec;
  if (!target.parent_path().empty()) {
    fs::create_directories(target.parent_path(), ec);
    if (ec) {
      if (error)
        *error =
            "cannot create parent directory: " + target.parent_path().string();
      return false;
    }
  }

  fs::path tmp = target;
  tmp += ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error)
        *error = "cannot open file for write: " + tmp.string();
      return false;
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) {
      if (error)
        *error = "cannot write file: " + tmp.string();
      return false;
    }
  }

  fs::rename(tmp, target, ec);
  if (ec) {
    std::error_code rm_ec;
    (void)fs::remove(tmp, rm_ec);
    if (error)
      *error = "cannot replace file: " + target.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline std::string
mdn_effective_runtime_report_run_id_(std::string_view run_id,
                                     std::string_view hashimyei) {
  if (!run_id.empty())
    return std::string(run_id);
  if (hashimyei.empty())
    return "run.mdn.unknown";
  return "run.mdn." + std::string(hashimyei);
}

[[nodiscard]] inline std::string
mdn_sanitize_runtime_history_token_(std::string_view token) {
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
mdn_runtime_history_file_path_(const std::filesystem::path &stable_file,
                               std::string_view history_token) {
  const std::string sanitized =
      mdn_sanitize_runtime_history_token_(history_token);
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

inline void mdn_append_runtime_string_entry_(runtime_lls_document_t *document,
                                             std::string_view key,
                                             std::string_view value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value)));
}

inline void
mdn_append_runtime_string_entry_if_nonempty_(runtime_lls_document_t *document,
                                             std::string_view key,
                                             std::string_view value) {
  if (!document || value.empty())
    return;
  mdn_append_runtime_string_entry_(document, key, value);
}

inline void mdn_append_runtime_u64_entry_(runtime_lls_document_t *document,
                                          std::string_view key,
                                          std::uint64_t value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value, "(0,+inf)"));
}

inline void mdn_append_runtime_i64_entry_(runtime_lls_document_t *document,
                                          std::string_view key,
                                          std::int64_t value) {
  if (!document)
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry(
          std::string(key), value));
}

inline void mdn_append_runtime_double_entry_(runtime_lls_document_t *document,
                                             std::string_view key,
                                             double value) {
  if (!document || !std::isfinite(value))
    return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          std::string(key), value));
}

[[nodiscard]] inline bool
save_wikimyei_inference_mdn_report_fragment_with_driver(
    const cuwacunu::hashimyei::report_fragment_action_context_t &action,
    std::string *error) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_mdn_hex_hash(action.report_fragment_id,
                                       &normalized_hashimyei)) {
    if (error)
      *error = "invalid mdn hashimyei id: " + action.report_fragment_id;
    return false;
  }

  auto *record =
      static_cast<wikimyei_inference_mdn_init_record_t *>(action.user_data);
  auto *model =
      static_cast<cuwacunu::wikimyei::ExpectedValue *>(action.object_handle);
  if (!model) {
    if (error)
      *error = "mdn report_fragment save requires ExpectedValue object";
    return false;
  }

  fs::path report_fragment_directory{};
  if (!resolve_wikimyei_inference_mdn_directory(
          normalized_hashimyei, false, &report_fragment_directory, error)) {
    return false;
  }

  std::error_code ec;
  fs::create_directories(report_fragment_directory, ec);
  if (ec) {
    if (error) {
      *error = "cannot create mdn report_fragment directory: " +
               report_fragment_directory.string();
    }
    return false;
  }

  const fs::path weights_file = report_fragment_directory / "weights.init.pt";
  fs::path network_analytics_file = weights_file;
  network_analytics_file.replace_extension(".network_analytics.lls");
  const fs::path status_lls_file =
      report_fragment_directory / "status.latest.lls";
  const std::string contract_hash =
      record ? record->contract_hash : std::string{};
  const std::string binding_id = record ? record->binding_id : std::string{};
  const std::string run_id = record ? record->run_id : std::string{};
  const bool has_wave_cursor = record && record->has_wave_cursor;
  const std::uint64_t wave_cursor = record ? record->wave_cursor : 0;
  const std::string canonical_path =
      std::string(kWikimyeiMdnCanonicalType) + "." + normalized_hashimyei;
  const std::string effective_run_id =
      mdn_effective_runtime_report_run_id_(run_id, normalized_hashimyei);
  std::vector<fs::path> history_files{};
  const auto write_history_copy = [&](const fs::path &stable_file) -> bool {
    std::error_code file_ec;
    if (!fs::exists(stable_file, file_ec) ||
        !fs::is_regular_file(stable_file, file_ec)) {
      return true;
    }
    const fs::path history_file =
        mdn_runtime_history_file_path_(stable_file, effective_run_id);
    if (history_file == stable_file)
      return true;
    fs::copy_file(stable_file, history_file,
                  fs::copy_options::overwrite_existing, file_ec);
    if (file_ec) {
      if (error)
        *error =
            "cannot persist mdn runtime history file: " + history_file.string();
      return false;
    }
    history_files.push_back(history_file);
    return true;
  };

  const bool write_network_analytics_sidecar =
      record && record->enable_network_analytics_sidecar;
  if (!model->save_checkpoint(weights_file.string(),
                              write_network_analytics_sidecar)) {
    if (error)
      *error = "mdn checkpoint save failed: " + weights_file.string();
    return false;
  }
  if (!write_history_copy(weights_file))
    return false;

  const bool wrote_network_analytics_file =
      fs::exists(network_analytics_file, ec) &&
      fs::is_regular_file(network_analytics_file, ec);
  if (wrote_network_analytics_file &&
      !write_history_copy(network_analytics_file))
    return false;

  const std::string canonical_action = action.canonical_action.empty()
                                           ? "tsi.wikimyei.inference.mdn.init()"
                                           : action.canonical_action;
  std::ostringstream metadata;
  metadata << "schema=tsi.wikimyei.inference.mdn.init.v1\n";
  metadata << "canonical_action=" << canonical_action << "\n";
  metadata << "canonical_target=" << kWikimyeiMdnCanonicalType << "\n";
  metadata << "family=" << kWikimyeiMdnFamily << "\n";
  metadata << "model=" << kWikimyeiMdnModel << "\n";
  metadata << "hashimyei=" << normalized_hashimyei << "\n";
  metadata << "canonical_path=" << canonical_path << "\n";
  metadata << "weights_file=" << weights_file.filename().string() << "\n";
  metadata << "status_file=" << status_lls_file.filename().string() << "\n";
  if (wrote_network_analytics_file) {
    metadata << "weights_network_analytics_file="
             << network_analytics_file.filename().string() << "\n";
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
    if (!write_wikimyei_mdn_text_file(report_fragment_directory /
                                          "metadata.txt",
                                      metadata.str(), &io_error)) {
      if (error) {
        *error =
            "cannot persist mdn metadata (encrypted failed: " + metadata_error +
            "; plaintext failed: " + io_error + ")";
      }
      return false;
    }
    metadata_plaintext_fallback = true;
    std::error_code stale_ec;
    (void)fs::remove(report_fragment_directory / "metadata.enc", stale_ec);
  }
  if (!write_history_copy(report_fragment_directory / "metadata.enc"))
    return false;
  if (!write_history_copy(report_fragment_directory / "metadata.txt"))
    return false;

  {
    const auto now_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    runtime_lls_document_t status_lls{};
    mdn_append_runtime_string_entry_(&status_lls, "schema",
                                     "tsi.wikimyei.inference.mdn.status.v1");
    {
      auto status_identity = make_component_report_identity(
          canonical_path, binding_id, "inference.network");
      status_identity.source_runtime_cursor =
          record ? record->source_runtime_cursor : std::string{};
      status_identity.has_wave_cursor = has_wave_cursor;
      status_identity.wave_cursor = wave_cursor;
      cuwacunu::piaabo::latent_lineage_state::
          append_runtime_report_header_entries(
              &status_lls, make_runtime_report_header(status_identity));
    }
    mdn_append_runtime_string_entry_(
        &status_lls, "report_event",
        report_event_token(report_event_e::ReportFragmentSave));
    mdn_append_runtime_string_entry_if_nonempty_(&status_lls, "contract_hash",
                                                 contract_hash);
    mdn_append_runtime_string_entry_if_nonempty_(
        &status_lls, "component_instance_name", model->component_name);
    mdn_append_runtime_u64_entry_(
        &status_lls, "trained_steps",
        static_cast<std::uint64_t>(
            std::max<std::int64_t>(0, model->total_iters_trained_)));
    mdn_append_runtime_u64_entry_(
        &status_lls, "optimizer_steps",
        static_cast<std::uint64_t>(
            std::max(0, model->runtime_state.optimizer_steps)));
    mdn_append_runtime_u64_entry_(
        &status_lls, "trained_epochs",
        static_cast<std::uint64_t>(
            std::max<std::int64_t>(0, model->total_epochs_trained_)));
    mdn_append_runtime_i64_entry_(&status_lls, "scheduler_batch_steps",
                                  model->scheduler_batch_steps_);
    mdn_append_runtime_i64_entry_(&status_lls, "scheduler_epoch_steps",
                                  model->scheduler_epoch_steps_);
    mdn_append_runtime_i64_entry_(&status_lls, "best_epoch",
                                  model->best_epoch_);
    mdn_append_runtime_double_entry_(&status_lls, "best_metric",
                                     model->best_metric_);
    mdn_append_runtime_double_entry_(&status_lls, "grad_clip",
                                     model->grad_clip);
    mdn_append_runtime_u64_entry_(
        &status_lls, "accumulate_steps",
        static_cast<std::uint64_t>(
            std::max(1, model->training_policy.accumulate_steps)));
    mdn_append_runtime_double_entry_(&status_lls, "clip_value",
                                     model->training_policy.clip_value);
    mdn_append_runtime_i64_entry_(&status_lls, "optimizer_threshold_reset",
                                  model->optimizer_threshold_reset);
    mdn_append_runtime_double_entry_(&status_lls, "last_committed_loss_mean",
                                     record ? record->last_committed_loss_mean
                                            : 0.0);
    mdn_append_runtime_i64_entry_(&status_lls, "epoch_optimizer_steps",
                                  record ? record->epoch_optimizer_steps : 0);
    mdn_append_runtime_string_entry_(&status_lls, "last_trial_id",
                                     effective_run_id);
    mdn_append_runtime_string_entry_(&status_lls, "last_contract_hash",
                                     contract_hash);
    mdn_append_runtime_u64_entry_(&status_lls, "updated_at_ms", now_ms);

    std::string status_payload;
    try {
      status_payload =
          cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
              status_lls);
    } catch (const std::exception &e) {
      if (error)
        *error = "cannot serialize mdn status report: " + std::string(e.what());
      return false;
    }
    std::string status_error;
    if (!write_wikimyei_mdn_text_file(status_lls_file, status_payload,
                                      &status_error)) {
      if (error)
        *error = "cannot persist mdn status report: " + status_error;
      return false;
    }
  }
  if (!write_history_copy(status_lls_file))
    return false;

  cuwacunu::hashimyei::report_fragment_manifest_t manifest{};
  manifest.family_canonical_path = std::string(kWikimyeiMdnCanonicalType);
  manifest.family = std::string(kWikimyeiMdnFamily);
  manifest.model = std::string(kWikimyeiMdnModel);
  manifest.report_fragment_id = normalized_hashimyei;
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
  append_manifest_file_if_present(weights_file);
  append_manifest_file_if_present(report_fragment_directory / "metadata.enc");
  append_manifest_file_if_present(report_fragment_directory / "metadata.txt");
  append_manifest_file_if_present(status_lls_file);
  if (wrote_network_analytics_file)
    append_manifest_file_if_present(network_analytics_file);
  for (const auto &history_file : history_files)
    append_manifest_file_if_present(history_file);

  std::string manifest_error;
  if (!cuwacunu::hashimyei::write_report_fragment_manifest(
          report_fragment_directory, manifest, &manifest_error)) {
    if (error)
      *error = "cannot persist mdn report_fragment manifest: " + manifest_error;
    return false;
  }

  if (record) {
    record->hashimyei = normalized_hashimyei;
    record->canonical_path = canonical_path;
    record->report_fragment_directory = report_fragment_directory;
    record->weights_file = weights_file;
    record->weights_count = 1;
    record->status_file = status_lls_file;
    record->network_analytics_file =
        wrote_network_analytics_file ? network_analytics_file : fs::path{};
    record->metadata_encrypted = metadata_encrypted;
    record->metadata_plaintext_fallback = metadata_plaintext_fallback;
    record->metadata_warning = metadata_warning;
  }
  return true;
}

[[nodiscard]] inline bool
load_wikimyei_inference_mdn_report_fragment_with_driver(
    const cuwacunu::hashimyei::report_fragment_action_context_t &action,
    std::string *error) {
  namespace fs = std::filesystem;
  if (error)
    error->clear();

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_mdn_hex_hash(action.report_fragment_id,
                                       &normalized_hashimyei)) {
    if (error)
      *error = "invalid mdn hashimyei id: " + action.report_fragment_id;
    return false;
  }

  fs::path report_fragment_directory{};
  if (!resolve_wikimyei_inference_mdn_directory(
          normalized_hashimyei, true, &report_fragment_directory, error)) {
    return false;
  }

  const fs::path weights_file = report_fragment_directory / "weights.init.pt";
  std::error_code ec;
  if (!fs::exists(weights_file, ec) || !fs::is_regular_file(weights_file, ec)) {
    if (error)
      *error = "mdn report_fragment weights file not found: " +
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
        *error = "cannot read mdn report_fragment manifest: " + manifest_error;
      return false;
    }
    if (manifest.family_canonical_path !=
        std::string(kWikimyeiMdnCanonicalType)) {
      if (error) {
        *error =
            "mdn report_fragment manifest family_canonical_path mismatch: " +
            manifest.family_canonical_path;
      }
      return false;
    }
    if (!action.family.empty() && manifest.family != action.family) {
      if (error)
        *error =
            "mdn report_fragment manifest family mismatch: " + manifest.family;
      return false;
    }
    if (!action.model.empty() && manifest.model != action.model) {
      if (error)
        *error =
            "mdn report_fragment manifest model mismatch: " + manifest.model;
      return false;
    }
    if (manifest.report_fragment_id != normalized_hashimyei) {
      if (error)
        *error = "mdn report_fragment manifest hashimyei mismatch: " +
                 manifest.report_fragment_id;
      return false;
    }
    if (!cuwacunu::hashimyei::report_fragment_manifest_has_file(
            manifest, weights_file.filename().string())) {
      if (error)
        *error = "mdn report_fragment manifest missing weights file entry: " +
                 weights_file.filename().string();
      return false;
    }
  }

  if (action.object_handle) {
    auto *model =
        static_cast<cuwacunu::wikimyei::ExpectedValue *>(action.object_handle);
    if (!model->load_checkpoint(weights_file.string())) {
      if (error)
        *error = "mdn checkpoint load failed: " + weights_file.string();
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool
ensure_wikimyei_inference_mdn_driver_registered(std::string *error = nullptr) {
  if (error)
    error->clear();
  if (cuwacunu::hashimyei::has_report_fragment_driver(
          kWikimyeiMdnCanonicalType))
    return true;

  cuwacunu::hashimyei::report_fragment_driver_t driver{};
  driver.family_canonical_path = std::string(kWikimyeiMdnCanonicalType);
  driver.family = std::string(kWikimyeiMdnFamily);
  driver.model = std::string(kWikimyeiMdnModel);
  driver.save = save_wikimyei_inference_mdn_report_fragment_with_driver;
  driver.load = load_wikimyei_inference_mdn_report_fragment_with_driver;

  if (cuwacunu::hashimyei::register_report_fragment_driver(std::move(driver),
                                                           error))
    return true;
  return cuwacunu::hashimyei::has_report_fragment_driver(
      kWikimyeiMdnCanonicalType);
}

[[nodiscard]] inline wikimyei_inference_mdn_init_record_t
update_wikimyei_inference_mdn_init(
    std::string hashimyei, cuwacunu::wikimyei::ExpectedValue *model,
    std::string contract_hash, std::string binding_id, std::string run_id,
    std::string source_runtime_cursor, bool has_wave_cursor,
    std::uint64_t wave_cursor, double last_committed_loss_mean,
    int epoch_optimizer_steps, bool enable_network_analytics_sidecar) {
  namespace fs = std::filesystem;
  wikimyei_inference_mdn_init_record_t out{};
  out.contract_hash = std::move(contract_hash);
  out.binding_id = std::move(binding_id);
  out.run_id = std::move(run_id);
  out.source_runtime_cursor = std::move(source_runtime_cursor);
  out.has_wave_cursor = has_wave_cursor;
  out.wave_cursor = wave_cursor;
  out.last_committed_loss_mean = last_committed_loss_mean;
  out.epoch_optimizer_steps = epoch_optimizer_steps;
  out.enable_network_analytics_sidecar = enable_network_analytics_sidecar;

  std::string registration_error;
  if (!ensure_wikimyei_inference_mdn_driver_registered(&registration_error)) {
    out.error =
        "failed to register mdn report_fragment driver: " + registration_error;
    return out;
  }

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_mdn_hex_hash(hashimyei, &normalized_hashimyei)) {
    out.error = "invalid mdn hashimyei id: " + hashimyei;
    return out;
  }

  out.hashimyei = std::move(normalized_hashimyei);
  if (!resolve_wikimyei_inference_mdn_directory(
          out.hashimyei, false, &out.report_fragment_directory, &out.error)) {
    return out;
  }

  std::error_code ec;
  fs::create_directories(out.report_fragment_directory, ec);
  if (ec) {
    out.error = "cannot create mdn report_fragment directory: " +
                out.report_fragment_directory.string();
    return out;
  }

  cuwacunu::hashimyei::report_fragment_action_context_t action{};
  action.family_canonical_path = std::string(kWikimyeiMdnCanonicalType);
  action.family = std::string(kWikimyeiMdnFamily);
  action.model = std::string(kWikimyeiMdnModel);
  action.report_fragment_id = out.hashimyei;
  action.report_fragment_directory = out.report_fragment_directory;
  action.canonical_action = "tsi.wikimyei.inference.mdn.init()";
  action.object_handle = model;
  action.user_data = &out;

  std::string dispatch_error;
  if (!cuwacunu::hashimyei::dispatch_report_fragment_save(
          kWikimyeiMdnCanonicalType, action, &dispatch_error)) {
    out.error = dispatch_error;
    return out;
  }

  out.ok = true;
  return out;
}

[[nodiscard]] inline bool load_wikimyei_inference_mdn_init_into_model(
    std::string_view hashimyei, const std::string &contract_hash,
    const std::string &component_name,
    std::unique_ptr<cuwacunu::wikimyei::ExpectedValue> *model_slot,
    std::string *error) {
  if (error)
    error->clear();
  if (!model_slot) {
    if (error)
      *error = "mdn model output slot is null";
    return false;
  }

  std::string registration_error;
  if (!ensure_wikimyei_inference_mdn_driver_registered(&registration_error)) {
    if (error)
      *error = "failed to register mdn report_fragment driver: " +
               registration_error;
    return false;
  }
  if (!*model_slot) {
    *model_slot = std::make_unique<cuwacunu::wikimyei::ExpectedValue>(
        contract_hash, component_name);
  }

  std::string normalized_hashimyei{};
  if (!normalize_wikimyei_mdn_hex_hash(hashimyei, &normalized_hashimyei)) {
    if (error)
      *error = "invalid mdn hashimyei id: " + std::string(hashimyei);
    return false;
  }

  cuwacunu::hashimyei::report_fragment_action_context_t action{};
  action.family_canonical_path = std::string(kWikimyeiMdnCanonicalType);
  action.family = std::string(kWikimyeiMdnFamily);
  action.model = std::string(kWikimyeiMdnModel);
  action.report_fragment_id = normalized_hashimyei;
  action.canonical_action = "tsi.wikimyei.inference.mdn.init()";
  action.object_handle = model_slot->get();

  return cuwacunu::hashimyei::dispatch_report_fragment_load(
      kWikimyeiMdnCanonicalType, action, error);
}

} // namespace tsiemene
