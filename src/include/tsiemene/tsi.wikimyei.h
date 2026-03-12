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
  bool enable_entropic_capacity_sidecar{true};
  std::filesystem::path entropic_capacity_file{};

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
