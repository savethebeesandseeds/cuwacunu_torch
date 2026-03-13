#include "piaabo/torch_compat/entropic_capacity_comparison.h"

#include "piaabo/latent_lineage_state/runtime_lls.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <exception>
#include <fstream>
#include <iomanip>
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
}

[[nodiscard]] std::optional<double> extract_numeric_kv_(
    std::string_view payload,
    std::string_view key) {
  runtime_lls_document_t document{};
  std::string parse_error;
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &document, &parse_error)) {
    return std::nullopt;
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, &parse_error)) {
    return std::nullopt;
  }
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

entropic_capacity_comparison_report_t summarize_entropic_capacity_comparison(
    const data_source_analytics_report_t& source,
    const network_analytics_report_t& network) {
  return summarize_entropic_capacity_comparison(
      source.source_entropic_load,
      network.network_global_entropic_capacity);
}

bool summarize_entropic_capacity_comparison_from_files(
    const std::filesystem::path& source_analytics_file,
    const std::filesystem::path& network_analytics_file,
    entropic_capacity_comparison_report_t* out,
    std::string* error) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "comparison output pointer is null";
    return false;
  }
  *out = entropic_capacity_comparison_report_t{};

  std::ifstream source_in(source_analytics_file, std::ios::binary);
  if (!source_in) {
    if (error) {
      *error = "cannot open source analytics file: " +
               source_analytics_file.string();
    }
    return false;
  }
  std::stringstream source_buf;
  source_buf << source_in.rdbuf();
  const std::string source_payload = source_buf.str();

  std::ifstream network_in(network_analytics_file, std::ios::binary);
  if (!network_in) {
    if (error) {
      *error = "cannot open network analytics file: " +
               network_analytics_file.string();
    }
    return false;
  }
  std::stringstream network_buf;
  network_buf << network_in.rdbuf();
  const std::string network_payload = network_buf.str();

  const auto source_load =
      extract_numeric_kv_(source_payload, "source_entropic_load");
  const auto network_capacity =
      extract_numeric_kv_(network_payload, "network_global_entropic_capacity");

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
  out->source_analytics_file = source_analytics_file.string();
  out->network_analytics_file = network_analytics_file.string();
  return true;
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
  if (!report.source_analytics_file.empty()) {
    document.entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "source_analytics_file", report.source_analytics_file));
  }
  if (!report.network_analytics_file.empty()) {
    document.entries.push_back(
        cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
            "network_analytics_file", report.network_analytics_file));
  }
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      document);
}

bool write_entropic_capacity_comparison_file(
    const entropic_capacity_comparison_report_t& report,
    const std::filesystem::path& output_file,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();

  std::error_code ec;
  const auto parent = output_file.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) {
        *error = "cannot create comparison output directory: " +
                 parent.string();
      }
      return false;
    }
  }

  std::ofstream out(output_file, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) {
      *error = "cannot open comparison output file: " + output_file.string();
    }
    return false;
  }
  try {
    out << entropic_capacity_comparison_to_latent_lineage_state_text(
        report, report_identity);
  } catch (const std::exception& e) {
    if (error) {
      *error = "cannot serialize comparison report: " + std::string(e.what());
    }
    return false;
  }
  if (!out.good()) {
    if (error) {
      *error = "cannot write comparison output file: " + output_file.string();
    }
    return false;
  }
  return true;
}

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
