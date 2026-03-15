// ./include/tsiemene/tsi.wikimyei.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include "tsiemene/tsi.h"

namespace tsiemene {

struct TsiWikimyeiInitRecord {
  bool ok{false};
  std::string error{};

  std::string hashimyei{};
  std::string contract_hash{};
  std::string run_id{};
  std::string canonical_base{};
  std::filesystem::path store_root{};
  std::filesystem::path report_fragment_directory{};
  std::filesystem::path weights_file{};
  bool enable_network_analytics_sidecar{true};

  bool metadata_encrypted{false};
  bool metadata_plaintext_fallback{false};
  std::string metadata_warning{};
};

struct TsiWikimyeiRuntimeIoContext {
  bool enable_debug_outputs{false};
  std::string run_id{};
};

struct TsiWikimyeiInitEntry {
  std::string hashimyei{};
  std::string canonical_base{};
  std::filesystem::path report_fragment_directory{};
  std::size_t weights_count{0};
};

struct TsiWikimyeiJkimyeiRuntimeState {
  std::string runtime_component_name{};
  std::string resolved_component_id{};
  std::string profile_id{};
  std::string profile_row_id{};
  std::string run_id{};

  bool train_enabled{false};
  bool use_swa{false};
  bool detach_to_cpu{false};
  bool supports_runtime_profile_switch{false};
  bool has_pending_grad{false};
  bool skip_on_nan{true};
  bool zero_grad_set_to_none{true};

  int optimizer_steps{0};
  int accumulate_steps{1};
  int accum_counter{0};
  int swa_start_iter{0};

  double clip_norm{0.0};
  double clip_value{0.0};
  double last_committed_loss_mean{0.0};
};

struct TsiWikimyeiJkimyeiFlushResult {
  bool had_pending_grad{false};
  bool optimizer_step_applied{false};
  bool has_pending_grad_after{false};
  double last_committed_loss_mean{0.0};
};

class TsiWikimyei : public Tsi {
 public:
  [[nodiscard]] TsiDomain domain() const noexcept final { return TsiDomain::Wikimyei; }
  [[nodiscard]] bool can_be_circuit_root() const noexcept override { return false; }
  [[nodiscard]] bool can_be_circuit_terminal() const noexcept override { return false; }
  [[nodiscard]] virtual bool supports_init_report_fragments() const noexcept {
    return false;
  }
  [[nodiscard]] virtual std::string_view init_report_fragment_schema() const noexcept {
    return {};
  }
  [[nodiscard]] virtual std::string_view report_fragment_family() const noexcept {
    return {};
  }
  [[nodiscard]] virtual std::string_view report_fragment_model() const noexcept {
    return {};
  }
  [[nodiscard]] virtual bool runtime_autoload_report_fragments() const noexcept {
    return supports_init_report_fragments();
  }
  [[nodiscard]] virtual bool runtime_autosave_report_fragments() const noexcept {
    return false;
  }
  [[nodiscard]] virtual bool supports_jkimyei_facet() const noexcept { return false; }
  [[nodiscard]] virtual bool jkimyei_get_runtime_state(
      TsiWikimyeiJkimyeiRuntimeState* out,
      std::string* error = nullptr) const {
    if (out) *out = TsiWikimyeiJkimyeiRuntimeState{};
    if (error) *error = "jkimyei facet unsupported";
    return false;
  }
  [[nodiscard]] virtual bool jkimyei_set_train_enabled(
      bool enabled,
      std::string* error = nullptr) {
    (void)enabled;
    if (error) *error = "jkimyei facet unsupported";
    return false;
  }
  [[nodiscard]] virtual bool jkimyei_flush_pending_training_step(
      TsiWikimyeiJkimyeiFlushResult* out = nullptr,
      std::string* error = nullptr) {
    if (out) *out = TsiWikimyeiJkimyeiFlushResult{};
    if (error) *error = "jkimyei facet unsupported";
    return false;
  }
  [[nodiscard]] virtual bool runtime_load_from_hashimyei(
      std::string_view hashimyei,
      std::string* error = nullptr,
      const void* runtime_context = nullptr) {
    (void)hashimyei;
    (void)runtime_context;
    if (error) error->clear();
    return false;
  }
  [[nodiscard]] virtual bool runtime_save_to_hashimyei(
      std::string_view hashimyei,
      std::string* error = nullptr,
      const void* runtime_context = nullptr) {
    (void)hashimyei;
    (void)runtime_context;
    if (error) error->clear();
    return false;
  }
  [[nodiscard]] bool allows_hop_to(const Tsi& downstream,
                                   DirectiveId out_directive,
                                   DirectiveId in_directive) const noexcept override {
    (void)out_directive;
    (void)in_directive;
    const TsiDomain d = downstream.domain();
    return d == TsiDomain::Wikimyei || d == TsiDomain::Sink || d == TsiDomain::Probe;
  }
  [[nodiscard]] bool allows_hop_from(const Tsi& upstream,
                                     DirectiveId out_directive,
                                     DirectiveId in_directive) const noexcept override {
    (void)out_directive;
    (void)in_directive;
    const TsiDomain u = upstream.domain();
    return u == TsiDomain::Source || u == TsiDomain::Wikimyei;
  }
};

} // namespace tsiemene
