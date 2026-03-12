// ./include/tsiemene/tsi.report.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace tsiemene {

// Report-dispatch vocabulary is intentionally run-centric to align with
// lattice-facing terminology.
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
  std::string report_kind{};
  std::string canonical_path{};
  std::string tsi_type{};
  std::string hashimyei{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string binding_id{};
  std::string run_id{};
};

[[nodiscard]] inline component_report_identity_t make_component_report_identity(
    std::string_view report_kind,
    std::string_view canonical_path,
    std::string_view tsi_type,
    std::string_view hashimyei = {},
    std::string_view contract_hash = {},
    std::string_view wave_hash = {},
    std::string_view binding_id = {},
    std::string_view run_id = {}) {
  component_report_identity_t out{};
  out.report_kind = std::string(report_kind);
  out.canonical_path = std::string(canonical_path);
  out.tsi_type = std::string(tsi_type);
  out.hashimyei = std::string(hashimyei);
  out.contract_hash = std::string(contract_hash);
  out.wave_hash = std::string(wave_hash);
  out.binding_id = std::string(binding_id);
  out.run_id = std::string(run_id);
  return out;
}

}  // namespace tsiemene
