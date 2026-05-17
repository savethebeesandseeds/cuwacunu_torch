#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "tsiemene/tsi.report.h"

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

inline constexpr std::string_view kEntropicCapacityComparisonSchemaV1 =
    "piaabo.torch_compat.entropic_capacity_comparison.v1";
inline constexpr std::string_view kEntropicCapacityComparisonSchemaCurrent =
    kEntropicCapacityComparisonSchemaV1;

struct entropic_capacity_comparison_report_t {
  std::string schema{std::string(kEntropicCapacityComparisonSchemaCurrent)};

  double source_entropic_load{0.0};
  double network_entropic_capacity{0.0};
  double capacity_margin{0.0};
  double capacity_ratio{0.0};

  std::string capacity_regime{"matched"};
};

[[nodiscard]] entropic_capacity_comparison_report_t
summarize_entropic_capacity_comparison(
    double source_entropic_load,
    double network_entropic_capacity);

[[nodiscard]] bool summarize_entropic_capacity_comparison_from_payloads(
    std::string_view source_analytics_payload,
    std::string_view network_analytics_payload,
    entropic_capacity_comparison_report_t* out,
    std::string* error = nullptr);

[[nodiscard]] bool summarize_entropic_capacity_comparison_from_kv_maps(
    const std::unordered_map<std::string, std::string>& source_kv,
    const std::unordered_map<std::string, std::string>& network_kv,
    entropic_capacity_comparison_report_t* out,
    std::string* error = nullptr);

[[nodiscard]] std::string entropic_capacity_comparison_to_latent_lineage_state_text(
    const entropic_capacity_comparison_report_t& report,
    const tsiemene::component_report_identity_t& report_identity = {});

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
