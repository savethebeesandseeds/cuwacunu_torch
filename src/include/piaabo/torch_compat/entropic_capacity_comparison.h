#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "piaabo/torch_compat/data_analytics.h"
#include "piaabo/torch_compat/network_analytics.h"

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
  std::string source_analytics_file{};
  std::string network_analytics_file{};
  std::string run_id{};
};

[[nodiscard]] entropic_capacity_comparison_report_t
summarize_entropic_capacity_comparison(
    double source_entropic_load,
    double network_entropic_capacity);

[[nodiscard]] entropic_capacity_comparison_report_t
summarize_entropic_capacity_comparison(
    const data_source_analytics_report_t& source,
    const network_analytics_report_t& network);

[[nodiscard]] bool summarize_entropic_capacity_comparison_from_files(
    const std::filesystem::path& source_analytics_file,
    const std::filesystem::path& network_analytics_file,
    entropic_capacity_comparison_report_t* out,
    std::string* error = nullptr);

[[nodiscard]] std::string entropic_capacity_comparison_to_latent_lineage_state_text(
    const entropic_capacity_comparison_report_t& report);

[[nodiscard]] bool write_entropic_capacity_comparison_file(
    const entropic_capacity_comparison_report_t& report,
    const std::filesystem::path& output_file,
    std::string* error = nullptr);

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
