#include "piaabo/torch_compat/data_analytics.h"

#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "piaabo/latent_lineage_state/runtime_lls.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cuwacunu {
namespace piaabo {
namespace torch_compat {

namespace {

constexpr double kNumericEpsilon = 1e-18;
constexpr std::size_t kDataAnalyticsContractHashPathLen = 8;
constexpr std::string_view kRefRangeNonNegative = "(0,+inf)";
constexpr std::string_view kRefRangePositive = "(1,+inf)";
constexpr std::string_view kRefRangeSigned = "(-inf,+inf)";

using runtime_lls_document_t =
    cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_t;

[[nodiscard]] inline double clamp_nonneg(double v) {
  return (v > 0.0) ? v : 0.0;
}

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

[[nodiscard]] std::string contract_hash_path_token_(std::string_view contract_hash) {
  std::string_view token = trim_ascii_ws_view_(contract_hash);
  if (token.size() >= 2 && token[0] == '0' &&
      (token[1] == 'x' || token[1] == 'X')) {
    token.remove_prefix(2);
  }
  if (token.empty()) return {};
  if (token.size() > kDataAnalyticsContractHashPathLen) {
    token = token.substr(0, kDataAnalyticsContractHashPathLen);
  }
  return std::string(token);
}

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

void append_string_entry_if_nonempty_(runtime_lls_document_t* document,
                                      std::string_view key,
                                      std::string_view value) {
  if (!document || value.empty()) return;
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value)));
}

void append_u64_entry_(runtime_lls_document_t* document,
                       std::string_view key,
                       std::uint64_t value,
                       std::string_view declared_domain = kRefRangeNonNegative) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value, std::string(declared_domain)));
}

void append_i64_entry_(runtime_lls_document_t* document,
                       std::string_view key,
                       std::int64_t value,
                       std::string_view declared_domain = kRefRangeSigned) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_int_entry(
          std::string(key), value, std::string(declared_domain)));
}

void append_double_entry_(runtime_lls_document_t* document,
                          std::string_view key,
                          double value,
                          std::string_view declared_domain = kRefRangeSigned) {
  document->entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          std::string(key), value, std::string(declared_domain)));
}

[[nodiscard]] data_analytics_options_t normalize_options_(
    const data_analytics_options_t& in) {
  data_analytics_options_t out = in;
  if (out.max_samples < 1) out.max_samples = 1;
  if (out.max_features < 1) out.max_features = 1;
  if (out.mask_epsilon < 0.0) out.mask_epsilon = 0.0;
  if (out.standardize_epsilon <= 0.0) out.standardize_epsilon = 1e-8;
  return out;
}

[[nodiscard]] inline bool tensor_is_finite_(const torch::Tensor& t) {
  if (!t.defined() || t.numel() == 0) return false;
  try {
    return torch::isfinite(t).all().item<bool>();
  } catch (...) {
    return false;
  }
}

[[nodiscard]] bool normalize_feature_and_mask_(
    const torch::Tensor& features,
    const torch::Tensor& mask,
    torch::Tensor* out_features,
    torch::Tensor* out_mask) {
  if (!out_features || !out_mask) return false;
  out_features->reset();
  out_mask->reset();

  if (!features.defined() || features.numel() == 0) return false;

  torch::Tensor f = features;
  if (f.dim() == 2) {
    f = f.unsqueeze(0).unsqueeze(0);  // [1,1,T,D]
  } else if (f.dim() == 3) {
    f = f.unsqueeze(0);  // [1,C,T,D]
  } else if (f.dim() != 4) {
    return false;
  }
  if (f.dim() != 4 || f.size(0) <= 0 || f.size(1) <= 0 || f.size(2) <= 0 ||
      f.size(3) <= 0) {
    return false;
  }

  torch::Tensor m;
  if (mask.defined() && mask.numel() > 0) {
    m = mask;
    if (m.dim() == 1) {
      m = m.unsqueeze(0).unsqueeze(0);  // [1,1,T]
    } else if (m.dim() == 2) {
      m = m.unsqueeze(0);  // [1,C,T]
    } else if (m.dim() != 3) {
      return false;
    }

    if (m.size(0) != f.size(0) || m.size(1) != f.size(1) || m.size(2) != f.size(2)) {
      return false;
    }

    m = m.to(torch::kCPU, torch::kFloat64);
  } else {
    m = torch::ones(
        {f.size(0), f.size(1), f.size(2)},
        torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
  }

  *out_features = f.to(torch::kCPU, torch::kFloat64);
  *out_mask = m;
  return true;
}

[[nodiscard]] torch::Tensor deterministic_feature_subsample_(
    const torch::Tensor& row,
    std::int64_t max_features) {
  if (!row.defined() || row.dim() != 1 || row.numel() <= 0) return torch::Tensor{};
  if (row.size(0) <= max_features) return row;

  if (max_features <= 1) {
    return row.slice(/*dim=*/0, /*start=*/0, /*end=*/1);
  }

  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(max_features));
  const double span = static_cast<double>(row.size(0) - 1);
  const double denom = static_cast<double>(max_features - 1);
  for (std::int64_t i = 0; i < max_features; ++i) {
    const double pos = (denom > 0.0) ? (span * static_cast<double>(i) / denom)
                                     : 0.0;
    std::int64_t id = static_cast<std::int64_t>(std::llround(pos));
    if (id < 0) id = 0;
    if (id >= row.size(0)) id = row.size(0) - 1;
    idx.push_back(id);
  }

  const auto index = torch::tensor(
      idx,
      torch::TensorOptions().dtype(torch::kLong).device(row.device()));
  return row.index_select(/*dim=*/0, index);
}

[[nodiscard]] bool extract_row_vectors_(
    const torch::Tensor& features,
    const torch::Tensor& mask,
    const data_analytics_options_t& options,
    std::vector<torch::Tensor>* out_rows,
    std::vector<double>* out_weights,
    std::uint64_t* out_sample_count,
    std::uint64_t* out_skipped_sample_count,
    std::int64_t* out_channels,
    std::int64_t* out_timesteps,
    std::int64_t* out_features_per_timestep,
    std::int64_t* out_flat_feature_count,
    std::int64_t* out_effective_feature_count) {
  if (!out_rows || !out_weights || !out_sample_count || !out_skipped_sample_count ||
      !out_channels || !out_timesteps || !out_features_per_timestep ||
      !out_flat_feature_count || !out_effective_feature_count) {
    return false;
  }

  torch::Tensor f;
  torch::Tensor m;
  if (!normalize_feature_and_mask_(features, mask, &f, &m)) {
    return false;
  }
  if (!tensor_is_finite_(f) || !tensor_is_finite_(m)) {
    return false;
  }

  const std::int64_t B = f.size(0);
  const std::int64_t C = f.size(1);
  const std::int64_t T = f.size(2);
  const std::int64_t D = f.size(3);
  const std::int64_t flat_features = C * T * D;

  *out_channels = C;
  *out_timesteps = T;
  *out_features_per_timestep = D;
  *out_flat_feature_count = flat_features;

  for (std::int64_t b = 0; b < B; ++b) {
    ++(*out_sample_count);

    torch::Tensor mask_b = m[b];
    torch::Tensor valid_b = mask_b > 0.0;
    double weight = 0.0;
    try {
      weight = valid_b.to(torch::kFloat64).mean().item<double>();
    } catch (...) {
      weight = 0.0;
    }

    if (!std::isfinite(weight) || weight <= options.mask_epsilon) {
      ++(*out_skipped_sample_count);
      continue;
    }

    torch::Tensor row = f[b].reshape({flat_features});
    row = deterministic_feature_subsample_(row, options.max_features);
    if (!row.defined() || row.dim() != 1 || row.numel() <= 0) {
      ++(*out_skipped_sample_count);
      continue;
    }

    if (!tensor_is_finite_(row)) {
      ++(*out_skipped_sample_count);
      continue;
    }

    if (static_cast<std::int64_t>(out_rows->size()) >= options.max_samples) {
      ++(*out_skipped_sample_count);
      continue;
    }

    if (*out_effective_feature_count == 0) {
      *out_effective_feature_count = row.size(0);
    } else if (*out_effective_feature_count != row.size(0)) {
      ++(*out_skipped_sample_count);
      continue;
    }

    out_rows->push_back(std::move(row));
    out_weights->push_back(weight);
  }

  return true;
}

[[nodiscard]] std::optional<double> parse_double_strict_(std::string_view s) {
  std::string text(trim_ascii_ws_view_(s));
  if (text.empty()) return std::nullopt;
  try {
    std::size_t pos = 0;
    const double v = std::stod(text, &pos);
    if (pos != text.size()) return std::nullopt;
    return v;
  } catch (...) {
    return std::nullopt;
  }
}

}  // namespace

data_source_analytics_accumulator_t::data_source_analytics_accumulator_t(
    data_analytics_options_t options)
    : options_(normalize_options_(options)) {}

void data_source_analytics_accumulator_t::reset() {
  rows_.clear();
  row_weights_.clear();
  sample_count_ = 0;
  skipped_sample_count_ = 0;
  source_channels_ = 0;
  source_timesteps_ = 0;
  source_features_per_timestep_ = 0;
  source_flat_feature_count_ = 0;
  source_effective_feature_count_ = 0;
}

bool data_source_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  std::int64_t channels = 0;
  std::int64_t timesteps = 0;
  std::int64_t features_per_timestep = 0;
  std::int64_t flat_feature_count = 0;
  std::int64_t effective_feature_count = source_effective_feature_count_;

  const bool ok = extract_row_vectors_(
      features,
      mask,
      options_,
      &rows_,
      &row_weights_,
      &sample_count_,
      &skipped_sample_count_,
      &channels,
      &timesteps,
      &features_per_timestep,
      &flat_feature_count,
      &effective_feature_count);
  if (!ok) {
    ++sample_count_;
    ++skipped_sample_count_;
    return false;
  }

  if (source_channels_ == 0 && channels > 0) source_channels_ = channels;
  if (source_timesteps_ == 0 && timesteps > 0) source_timesteps_ = timesteps;
  if (source_features_per_timestep_ == 0 && features_per_timestep > 0) {
    source_features_per_timestep_ = features_per_timestep;
  }
  if (source_flat_feature_count_ == 0 && flat_feature_count > 0) {
    source_flat_feature_count_ = flat_feature_count;
  }
  if (effective_feature_count > 0) {
    source_effective_feature_count_ = effective_feature_count;
  }

  return true;
}

data_source_analytics_report_t data_source_analytics_accumulator_t::summarize()
    const {
  data_source_analytics_report_t out{};
  out.sample_count = sample_count_;
  out.skipped_sample_count = skipped_sample_count_;
  out.valid_sample_count = static_cast<std::uint64_t>(rows_.size());
  out.source_channels = source_channels_;
  out.source_timesteps = source_timesteps_;
  out.source_features_per_timestep = source_features_per_timestep_;
  out.source_flat_feature_count = source_flat_feature_count_;
  out.source_effective_feature_count = source_effective_feature_count_;

  if (rows_.empty()) return out;
  if (rows_.size() != row_weights_.size()) return out;

  torch::Tensor X;
  torch::Tensor w;
  try {
    X = torch::stack(rows_, /*dim=*/0);  // [N,F]
    w = torch::tensor(
        row_weights_,
        torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
  } catch (...) {
    return out;
  }

  if (!X.defined() || X.dim() != 2 || X.size(0) <= 0 || X.size(1) <= 0) {
    return out;
  }
  if (!w.defined() || w.dim() != 1 || w.size(0) != X.size(0)) {
    return out;
  }

  try {
    w = w.clamp_min(0.0);
    const double w_sum = w.sum().item<double>();
    if (!std::isfinite(w_sum) || w_sum <= kNumericEpsilon) return out;

    const torch::Tensor w_col = w.unsqueeze(1);  // [N,1]
    const torch::Tensor mean = (X * w_col).sum(/*dim=*/0) / w_sum;

    const torch::Tensor centered = X - mean;
    const torch::Tensor var = (centered * centered * w_col).sum(/*dim=*/0) / w_sum;
    const torch::Tensor stdev = torch::sqrt(var.clamp_min(options_.standardize_epsilon));
    const torch::Tensor Z = centered / stdev;

    const torch::Tensor ws = torch::sqrt(w_col);
    const torch::Tensor Zw = Z * ws;
    const torch::Tensor cov = Zw.transpose(0, 1).mm(Zw) / w_sum;

    torch::Tensor eigvals = torch::linalg::eigvalsh(cov, "L");
    eigvals = eigvals.clamp_min(0.0);

    const double trace = eigvals.sum().item<double>();
    out.source_cov_trace = std::isfinite(trace) ? trace : 0.0;

    if (!std::isfinite(trace) || trace <= kNumericEpsilon) return out;

    const torch::Tensor probs = eigvals / trace;
    const torch::Tensor probs_safe = probs.clamp_min(kNumericEpsilon);
    const double entropy =
        (-probs * torch::log(probs_safe)).sum().item<double>();

    if (std::isfinite(entropy)) {
      out.source_entropic_load = std::exp(entropy);
    }

    out.source_nonzero_eigen_count = static_cast<std::uint64_t>(
        (eigvals > options_.standardize_epsilon).sum().item<std::int64_t>());
  } catch (...) {
    return out;
  }

  return out;
}

std::string data_analytics_to_latent_lineage_state_text(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    std::string_view source_label,
    const tsiemene::component_report_identity_t& report_identity) {
  runtime_lls_document_t document{};
  document.entries.reserve(18);
  document.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          "schema", report.schema));
  append_component_report_identity_entries_(&document, report_identity);
  append_string_entry_if_nonempty_(&document, "source_label", source_label);

  append_u64_entry_(&document, "sample_count", report.sample_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document, "valid_sample_count", report.valid_sample_count, kRefRangeNonNegative);
  append_u64_entry_(
      &document, "skipped_sample_count", report.skipped_sample_count, kRefRangeNonNegative);

  append_i64_entry_(&document, "source_channels", report.source_channels, kRefRangeNonNegative);
  append_i64_entry_(
      &document, "source_timesteps", report.source_timesteps, kRefRangeNonNegative);
  append_i64_entry_(
      &document,
      "source_features_per_timestep",
      report.source_features_per_timestep,
      kRefRangeNonNegative);
  append_i64_entry_(
      &document, "source_flat_feature_count", report.source_flat_feature_count, kRefRangeNonNegative);
  append_i64_entry_(
      &document,
      "source_effective_feature_count",
      report.source_effective_feature_count,
      kRefRangeNonNegative);

  append_double_entry_(
      &document, "source_entropic_load", report.source_entropic_load, kRefRangeNonNegative);
  append_double_entry_(
      &document, "source_cov_trace", report.source_cov_trace, kRefRangeNonNegative);
  append_u64_entry_(
      &document,
      "source_nonzero_eigen_count",
      report.source_nonzero_eigen_count,
      kRefRangeNonNegative);

  append_i64_entry_(&document, "max_samples", options.max_samples, kRefRangePositive);
  append_i64_entry_(&document, "max_features", options.max_features, kRefRangePositive);
  append_double_entry_(&document, "mask_epsilon", options.mask_epsilon, kRefRangeNonNegative);
  append_double_entry_(
      &document, "standardize_epsilon", options.standardize_epsilon, kRefRangeNonNegative);

  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      document);
}

bool write_data_analytics_file(
    const data_source_analytics_report_t& report,
    const data_analytics_options_t& options,
    const std::filesystem::path& output_file,
    std::string_view source_label,
    std::string* error,
    const tsiemene::component_report_identity_t& report_identity) {
  if (error) error->clear();

  std::error_code ec;
  const auto parent = output_file.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) {
        *error = "cannot create data analytics directory: " + parent.string();
      }
      return false;
    }
  }

  std::ofstream out(output_file, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot open output file: " + output_file.string();
    return false;
  }

  std::string payload;
  try {
    payload = data_analytics_to_latent_lineage_state_text(
        report, options, source_label, report_identity);
  } catch (const std::exception& e) {
    if (error) *error = "cannot serialize data analytics report: " + std::string(e.what());
    return false;
  }
  out << payload;
  if (!out.good()) {
    if (error) *error = "cannot write output file: " + output_file.string();
    return false;
  }
  return true;
}

std::string extract_data_analytics_kv_schema(std::string_view payload) {
  runtime_lls_document_t document{};
  std::string parse_error;
  if (!cuwacunu::piaabo::latent_lineage_state::parse_runtime_lls_text(
          payload, &document, &parse_error)) {
    return {};
  }
  std::unordered_map<std::string, std::string> kv{};
  if (!cuwacunu::piaabo::latent_lineage_state::runtime_lls_document_to_kv_map(
          document, &kv, &parse_error)) {
    return {};
  }
  if (const auto it = kv.find("schema"); it != kv.end()) return it->second;
  return {};
}

bool is_supported_data_analytics_schema(std::string_view schema) {
  return schema == kDataAnalyticsSchemaV1;
}

std::filesystem::path source_data_analytics_root_directory() {
  return cuwacunu::hashimyei::store_root() / "tsi.source" / "data_analytics";
}

std::filesystem::path source_data_analytics_contract_directory(
    std::string_view contract_hash) {
  const std::string token = contract_hash_path_token_(contract_hash);
  if (token.empty()) return {};
  return source_data_analytics_root_directory() / token;
}

std::filesystem::path source_data_analytics_instance_directory(
    std::string_view contract_hash,
    std::string_view source_instance) {
  if (contract_hash.empty() || source_instance.empty()) return {};
  return source_data_analytics_contract_directory(contract_hash) /
         std::string(source_instance);
}

std::filesystem::path source_data_analytics_latest_file_path(
    std::string_view contract_hash,
    std::string_view source_instance) {
  const auto base = source_data_analytics_instance_directory(
      contract_hash,
      source_instance);
  if (base.empty()) return {};
  return base / std::string(kDataAnalyticsLatestReportFilename);
}

}  // namespace torch_compat
}  // namespace piaabo
}  // namespace cuwacunu
