#include "piaabo/torch_compat/entropic_capacity_comparison.h"

#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

namespace {

constexpr double kNumericEpsilon = 1e-18;

[[nodiscard]] std::string_view trim_ascii_ws_view_(std::string_view text) {
  std::size_t begin = 0;
  std::size_t end = text.size();
  while (begin < end &&
         std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return text.substr(begin, end - begin);
}

[[nodiscard]] std::optional<double> extract_numeric_kv_(
    std::string_view payload,
    std::string_view key) {
  std::size_t cursor = 0;
  while (cursor < payload.size()) {
    std::size_t line_end = payload.find('\n', cursor);
    if (line_end == std::string_view::npos) line_end = payload.size();

    std::string_view line = payload.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    line = trim_ascii_ws_view_(line);

    if (!line.empty()) {
      const std::size_t sep = line.find('=');
      if (sep != std::string_view::npos) {
        const std::string parsed_key =
            cuwacunu::camahjucunu::dsl::extract_latent_lineage_state_lhs_key(
                line.substr(0, sep));
        if (parsed_key == key) {
          const std::string value(trim_ascii_ws_view_(line.substr(sep + 1)));
          if (value.empty()) return std::nullopt;
          try {
            std::size_t pos = 0;
            const double numeric = std::stod(value, &pos);
            if (pos != value.size()) return std::nullopt;
            return numeric;
          } catch (...) {
            return std::nullopt;
          }
        }
      }
    }

    if (line_end == payload.size()) break;
    cursor = line_end + 1;
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
    const entropic_capacity_comparison_report_t& report) {
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss << std::setprecision(10);

  oss << "schema=" << report.schema << "\n";
  oss << "source_entropic_load=" << report.source_entropic_load << "\n";
  oss << "network_entropic_capacity=" << report.network_entropic_capacity << "\n";
  oss << "capacity_margin=" << report.capacity_margin << "\n";
  oss << "capacity_ratio=" << report.capacity_ratio << "\n";
  oss << "capacity_regime=" << report.capacity_regime << "\n";
  if (!report.run_id.empty()) {
    oss << "run_id=" << report.run_id << "\n";
  }
  if (!report.source_analytics_file.empty()) {
    oss << "source_analytics_file=" << report.source_analytics_file << "\n";
  }
  if (!report.network_analytics_file.empty()) {
    oss << "network_analytics_file=" << report.network_analytics_file << "\n";
  }
  return cuwacunu::camahjucunu::dsl::convert_latent_lineage_state_payload_to_lattice_state(
      oss.str());
}

bool write_entropic_capacity_comparison_file(
    const entropic_capacity_comparison_report_t& report,
    const std::filesystem::path& output_file,
    std::string* error) {
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
  out << entropic_capacity_comparison_to_latent_lineage_state_text(report);
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
