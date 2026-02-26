// ./include/tsiemene/tsi.source.h
// SPDX-License-Identifier: MIT
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "tsiemene/tsi.h"

namespace tsiemene {

struct TsiSourceInitRecord {
  bool ok{false};
  std::string error{};

  std::string init_id{};
  std::filesystem::path store_root{};
  std::filesystem::path init_directory{};

  bool metadata_encrypted{false};
  bool metadata_plaintext_fallback{false};
  std::string metadata_warning{};
};

struct TsiSourceInitEntry {
  std::string init_id{};
  std::filesystem::path init_directory{};
};

class TsiSource : public Tsi {
 public:
  [[nodiscard]] TsiDomain domain() const noexcept final { return TsiDomain::Source; }
  [[nodiscard]] bool can_be_circuit_root() const noexcept override { return true; }
  [[nodiscard]] bool can_be_circuit_terminal() const noexcept override { return false; }
  [[nodiscard]] virtual bool supports_init_artifacts() const noexcept { return false; }
  [[nodiscard]] virtual std::string_view init_artifact_schema() const noexcept { return {}; }
  [[nodiscard]] bool allows_hop_to(const Tsi& downstream,
                                   DirectiveId out_directive,
                                   DirectiveId in_directive) const noexcept override {
    (void)out_directive;
    (void)in_directive;
    const TsiDomain d = downstream.domain();
    return d == TsiDomain::Wikimyei || d == TsiDomain::Sink;
  }
  [[nodiscard]] bool allows_hop_from(const Tsi& upstream,
                                     DirectiveId out_directive,
                                     DirectiveId in_directive) const noexcept override {
    (void)upstream;
    (void)out_directive;
    (void)in_directive;
    // Sources are expected to be circuit roots.
    return false;
  }
};

} // namespace tsiemene
