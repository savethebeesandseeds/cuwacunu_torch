// ./include/tsiemene/tsi.report.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "piaabo/latent_lineage_state/runtime_lls.h"

namespace tsiemene {

// Report-dispatch vocabulary is component/fact centric. Runtime report identity
// is carried by semantic_taxon plus the compact context header.
enum class report_event_e : std::uint8_t {
  Step = 0,
  EpochStart = 1,
  EpochEnd = 2,
  RunStart = 3,
  RunEnd = 4,
  ReportFragmentLoad = 5,
  ReportFragmentSave = 6,
};

[[nodiscard]] constexpr std::string_view report_event_token(
    report_event_e event) noexcept {
  switch (event) {
    case report_event_e::Step:
      return "step";
    case report_event_e::EpochStart:
      return "epoch_start";
    case report_event_e::EpochEnd:
      return "epoch_end";
    case report_event_e::RunStart:
      return "run_start";
    case report_event_e::RunEnd:
      return "run_end";
    case report_event_e::ReportFragmentLoad:
      return "report_fragment_load";
    case report_event_e::ReportFragmentSave:
      return "report_fragment_save";
  }
  return "unknown";
}

struct component_report_identity_t {
  std::string canonical_path{};
  std::string semantic_taxon{};
  std::string binding_id{};
  std::string source_runtime_cursor{};
  bool has_wave_cursor{false};
  std::uint64_t wave_cursor{0};
};

[[nodiscard]] inline cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t
make_runtime_report_header(const component_report_identity_t& identity) {
  cuwacunu::piaabo::latent_lineage_state::runtime_report_header_t out{};
  out.semantic_taxon = identity.semantic_taxon;
  out.context.canonical_path = identity.canonical_path;
  out.context.binding_id = identity.binding_id;
  out.context.source_runtime_cursor = identity.source_runtime_cursor;
  out.context.has_wave_cursor = identity.has_wave_cursor;
  out.context.wave_cursor = identity.wave_cursor;
  return out;
}

[[nodiscard]] inline component_report_identity_t make_component_report_identity(
    std::string_view canonical_path,
    std::string_view binding_id = {},
    std::string_view semantic_taxon = {}) {
  component_report_identity_t out{};
  out.canonical_path = std::string(canonical_path);
  out.semantic_taxon = std::string(semantic_taxon);
  out.binding_id = std::string(binding_id);
  return out;
}

}  // namespace tsiemene
