#include "piaabo/torch_compat/entropic_capacity_comparison.h"

#include "piaabo/latent_lineage_state/runtime_lls.h"

#include <cmath>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

namespace {

constexpr double kNumericEpsilon = 1e-18;
constexpr std::string_view kRefRangeNonNegative = "(0,+inf)";
constexpr std::string_view kRefRangeSigned = "(-inf,+inf)";
constexpr std::string_view kCapacityRegimeDomain =
    "[under_capacity,matched,surplus_capacity]";

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

void append_component_report_identity_entries_(
    runtime_lls_document_t* document,
    const tsiemene::component_report_identity_t& report_identity) {
  if (!document) return;
  if (!report_identity.report_kind.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "report_kind", report_identity.report_kind));
  }
  if (!report_identity.tsi_type.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "tsi_type", report_identity.tsi_type));
  }
  if (!report_identity.canonical_path.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "canonical_path", report_identity.canonical_path));
  }
  if (!report_identity.hashimyei.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "hashimyei", report_identity.hashimyei));
  }
  if (!report_identity.contract_hash.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "contract_hash", report_identity.contract_hash));
  }
  if (!report_identity.wave_hash.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "wave_hash", report_identity.wave_hash));
  }
  if (!report_identity.binding_id.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "binding_id", report_identity.binding_id));
  }
  if (!report_identity.run_id.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "run_id", report_identity.run_id));
  }
  if (!report_identity.wave_cursor_resolution.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "wave_cursor_resolution", report_identity.wave_cursor_resolution));
  }
  if (!report_identity.intersection_cursor.empty()) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "intersection_cursor", report_identity.intersection_cursor));
  }
  if (report_identity.has_wave_cursor) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave_cursor", report_identity.wave_cursor, "[0,+inf)"));
  }
  if (report_identity.has_wave_cursor_run) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave.cursor.run", report_identity.wave_cursor_run, "[0,+inf)"));
  }
  if (report_identity.has_wave_cursor_episode) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave.cursor.episode",
            report_identity.wave_cursor_episode,
            "[0,+inf)"));
  }
  if (report_identity.has_wave_cursor_batch) {
    document->entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
            "wave.cursor.batch", report_identity.wave_cursor_batch, "[0,+inf)"));
  }
}

[[nodiscard]] std::optional<double> extract_numeric_kv_(
    const std::unordered_map<std::string, std::string>& kv,
    std::string_view key) {
  const auto it = kv.find(std::string(key));
  if (it == kv.end() || it->second.empty()) return std::nullopt;
  try {
    std::size_t pos = 0;
    const double numeric = std::stod(it->second, &pos);
    if (pos != it->second.size()) return std::nullopt;
    return numeric;
  } catch (...) {
    return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::string classify_capacity_regime_(double ratio) {
  if (!std::isfinite(ratio)) return "matched";
  if (ratio < 0.8) return "under_capacity";
  if (ratio > 1.5) return "surplus_capacity";
  return "matched";
}

}  // namespace

entropic_capacity_comparison_report_t summarize_entropic_capacity_comparison(
    double source_entropic_load,
    double network_entropic_capacity) {
  entropic_capacity_comparison_report_t out{};
  out.source_entropic_load =
      std::isfinite(source_entropic_load) ? source_entropic_load : 0.0;
  out.network_entropic_capacity =
      std::isfinite(network_entropic_capacity) ? network_entropic_capacity : 0.0;
  out.capacity_margin = out.network_entropic_capacity - out.source_entropic_load;
  out.capacity_ratio =
      out.network_entropic_capacity / (out.source_entropic_load + kNumericEpsilon);
  out.capacity_regime = classify_capacity_regime_(out.capacity_ratio);
  return out;
}

bool summarize_entropic_capacity_comparison_from_kv_maps(
    const std::unordered_map<std::string, std::string>& source_kv,
    const std::unordered_map<std::string, std::string>& network_kv,
    entropic_capacity_comparison_report_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "comparison output pointer is null";
    return false;
  }
  *out = entropic_capacity_comparison_report_t{};

  const auto source_load = extract_numeric_kv_(source_kv, "source_entropic_load");
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
      *error =
          "network analytics key missing or invalid: network_global_entropic_capacity";
    }
    return false;
  }

  *out = summarize_entropic_capacity_comparison(*source_load, *network_capacity);
  if (const auto it = source_kv.find("run_id");
      it != source_kv.end() && !it->second.empty()) {
    out->run_id = it->second;
  } else if (const auto it = network_kv.find("run_id");
             it != network_kv.end() && !it->second.empty()) {
    out->run_id = it->second;
  }
  return true;
}

bool summarize_entropic_capacity_comparison_from_payloads(
    std::string_view source_analytics_payload,
    std::string_view network_analytics_payload,
    entropic_capacity_comparison_report_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "comparison output pointer is null";
    return false;
  }
  *out = entropic_capacity_comparison_report_t{};

  runtime_lls_document_t source_document{};
  runtime_lls_document_t network_document{};
  std::unordered_map<std::string, std::string> source_kv{};
  std::unordered_map<std::string, std::string> network_kv{};
  std::string parse_error{};
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          source_analytics_payload, &source_document, &parse_error) ||
      !cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          source_document, &source_kv, &parse_error)) {
    if (error) *error = "invalid source analytics payload: " + parse_error;
    return false;
  }
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          network_analytics_payload, &network_document, &parse_error) ||
      !cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          network_document, &network_kv, &parse_error)) {
    if (error) *error = "invalid network analytics payload: " + parse_error;
    return false;
  }
  return summarize_entropic_capacity_comparison_from_kv_maps(
      source_kv, network_kv, out, error);
}

std::string entropic_capacity_comparison_to_latent_lineage_state_text(
    const entropic_capacity_comparison_report_t& report,
    const tsiemene::component_report_identity_t& report_identity) {
  runtime_lls_document_t document{};
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          "schema", report.schema));
  append_component_report_identity_entries_(&document, report_identity);
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "source_entropic_load",
          report.source_entropic_load,
          std::string(kRefRangeNonNegative)));
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "network_entropic_capacity",
          report.network_entropic_capacity,
          std::string(kRefRangeNonNegative)));
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "capacity_margin", report.capacity_margin, std::string(kRefRangeSigned)));
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          "capacity_ratio",
          report.capacity_ratio,
          std::string(kRefRangeNonNegative)));
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_entry(
          "capacity_regime",
          report.capacity_regime,
          "str",
          std::string(kCapacityRegimeDomain)));
  if (!report.run_id.empty() && report_identity.run_id.empty()) {
    document.entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "run_id", report.run_id));
  }
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      document);
}

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
