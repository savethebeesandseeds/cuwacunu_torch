#include "jkimyei/evaluation/inference/entropic_capacity_comparison.h"

#include "hero/lattice_hero/lattice/runtime_report/runtime_lls.h"

#include <cmath>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace cuwacunu {
namespace jkimyei {
namespace evaluation {

namespace {

constexpr double kNumericEpsilon = 1e-18;
constexpr std::string_view kRefRangeNonNegative = "(0,+inf)";
constexpr std::string_view kRefRangeSigned = "(-inf,+inf)";
constexpr std::string_view kCapacityRegimeDomain =
    "[under_capacity,matched,surplus_capacity]";

using runtime_lls_document_t = cuwacunu::hero::lattice::runtime_report::runtime_lls_document_t;

void append_component_report_identity_entries_(
    runtime_lls_document_t *document,
    const evaluation_report_identity_t &report_identity) {
  append_evaluation_report_identity_entries(document, report_identity);
}

[[nodiscard]] std::optional<double>
extract_numeric_kv_(const std::unordered_map<std::string, std::string> &kv,
                    std::string_view key) {
  const auto it = kv.find(std::string(key));
  if (it == kv.end() || it->second.empty())
    return std::nullopt;
  try {
    std::size_t pos = 0;
    const double numeric = std::stod(it->second, &pos);
    if (pos != it->second.size())
      return std::nullopt;
    return numeric;
  } catch (...) {
    return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::string classify_capacity_regime_(double ratio) {
  if (!std::isfinite(ratio))
    return "matched";
  if (ratio < 0.8)
    return "under_capacity";
  if (ratio > 1.5)
    return "surplus_capacity";
  return "matched";
}

} // namespace

entropic_capacity_comparison_report_t
summarize_entropic_capacity_comparison(double source_entropic_load,
                                       double network_entropic_capacity) {
  entropic_capacity_comparison_report_t out{};
  out.source_entropic_load =
      std::isfinite(source_entropic_load) ? source_entropic_load : 0.0;
  out.network_entropic_capacity = std::isfinite(network_entropic_capacity)
                                      ? network_entropic_capacity
                                      : 0.0;
  out.capacity_margin =
      out.network_entropic_capacity - out.source_entropic_load;
  out.capacity_ratio = out.network_entropic_capacity /
                       (out.source_entropic_load + kNumericEpsilon);
  out.capacity_regime = classify_capacity_regime_(out.capacity_ratio);
  return out;
}

bool summarize_entropic_capacity_comparison_from_kv_maps(
    const std::unordered_map<std::string, std::string> &source_kv,
    const std::unordered_map<std::string, std::string> &network_kv,
    entropic_capacity_comparison_report_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "comparison output pointer is null";
    return false;
  }
  *out = entropic_capacity_comparison_report_t{};

  const auto source_load =
      extract_numeric_kv_(source_kv, "source_entropic_load");
  const auto network_capacity =
      extract_numeric_kv_(network_kv, "network_global_entropic_capacity");
  if (!source_load.has_value()) {
    if (error) {
      *error = "source analytics key missing or invalid: source_entropic_load";
    }
    return false;
  }
  if (!network_capacity.has_value()) {
    if (error) {
      *error = "network analytics key missing or invalid: "
               "network_global_entropic_capacity";
    }
    return false;
  }

  *out =
      summarize_entropic_capacity_comparison(*source_load, *network_capacity);
  return true;
}

bool summarize_entropic_capacity_comparison_from_payloads(
    std::string_view source_analytics_payload,
    std::string_view network_analytics_payload,
    entropic_capacity_comparison_report_t *out, std::string *error) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "comparison output pointer is null";
    return false;
  }
  *out = entropic_capacity_comparison_report_t{};

  runtime_lls_document_t source_document{};
  runtime_lls_document_t network_document{};
  std::unordered_map<std::string, std::string> source_kv{};
  std::unordered_map<std::string, std::string> network_kv{};
  std::string parse_error{};
  if (!cuwacunu::hero::lattice::runtime_report::
          parse_runtime_lls_text(source_analytics_payload, &source_document,
                                 &parse_error) ||
      !cuwacunu::hero::lattice::runtime_report::
          runtime_lls_document_to_kv_map(source_document, &source_kv,
                                         &parse_error)) {
    if (error)
      *error = "invalid source analytics payload: " + parse_error;
    return false;
  }
  if (!cuwacunu::hero::lattice::runtime_report::
          parse_runtime_lls_text(network_analytics_payload, &network_document,
                                 &parse_error) ||
      !cuwacunu::hero::lattice::runtime_report::
          runtime_lls_document_to_kv_map(network_document, &network_kv,
                                         &parse_error)) {
    if (error)
      *error = "invalid network analytics payload: " + parse_error;
    return false;
  }
  return summarize_entropic_capacity_comparison_from_kv_maps(
      source_kv, network_kv, out, error);
}

std::string entropic_capacity_comparison_to_latent_lineage_state_text(
    const entropic_capacity_comparison_report_t &report,
    const evaluation_report_identity_t &report_identity) {
  runtime_lls_document_t document{};
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_string_entry("schema", report.schema));
  append_component_report_identity_entries_(&document, report_identity);
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_double_entry("source_entropic_load",
                                        report.source_entropic_load,
                                        std::string(kRefRangeNonNegative)));
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_double_entry("network_entropic_capacity",
                                        report.network_entropic_capacity,
                                        std::string(kRefRangeNonNegative)));
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_double_entry("capacity_margin",
                                        report.capacity_margin,
                                        std::string(kRefRangeSigned)));
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_string_entry("capacity_margin_preference",
                                        "higher_is_better"));
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_double_entry("capacity_ratio", report.capacity_ratio,
                                        std::string(kRefRangeNonNegative)));
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_string_entry("capacity_ratio_reference",
                                        "1.0=matched_load_capacity"));
  document.entries.push_back(
      cuwacunu::hero::lattice::runtime_report::
          make_runtime_lls_entry("capacity_regime", report.capacity_regime,
                                 "str", std::string(kCapacityRegimeDomain)));
  return cuwacunu::hero::lattice::runtime_report::
      emit_runtime_lls_canonical(document);
}

} // namespace evaluation
} // namespace jkimyei
} // namespace cuwacunu
