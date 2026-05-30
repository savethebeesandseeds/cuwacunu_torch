struct extracted_sequence_rows_t {
  std::vector<torch::Tensor> rows{};
  std::vector<torch::Tensor> validity_masks{};
  std::uint64_t sample_count{0};
  std::uint64_t skipped_sample_count{0};
  std::int64_t channels{0};
  std::int64_t timesteps{0};
  std::int64_t features_per_timestep{0};
  std::int64_t flat_feature_count{0};
  std::int64_t sampled_feature_count{0};
};

[[nodiscard]] torch::Tensor deterministic_feature_subsample_index_(
    std::int64_t feature_count,
    std::int64_t max_features) {
  if (feature_count <= 0 || max_features <= 0) return torch::Tensor{};
  if (feature_count <= max_features) {
    return torch::arange(
        feature_count,
        torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
  }

  if (max_features <= 1) {
    return torch::zeros(
        {1}, torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
  }

  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(max_features));
  const double span = static_cast<double>(feature_count - 1);
  const double denom = static_cast<double>(max_features - 1);
  for (std::int64_t i = 0; i < max_features; ++i) {
    const double pos = (denom > 0.0) ? (span * static_cast<double>(i) / denom)
                                     : 0.0;
    std::int64_t id = static_cast<std::int64_t>(std::llround(pos));
    if (id < 0) id = 0;
    if (id >= feature_count) id = feature_count - 1;
    idx.push_back(id);
  }

  return torch::tensor(
      idx,
      torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
}

[[nodiscard]] torch::Tensor sample_feature_vector_(
    const torch::Tensor& row,
    const torch::Tensor& index) {
  if (!row.defined() || row.dim() != 1 || row.numel() <= 0) return torch::Tensor{};
  if (!index.defined() || index.dim() != 1 || index.numel() <= 0) {
    return torch::Tensor{};
  }
  if (index.size(0) == row.size(0)) return row;
  return row.index_select(/*dim=*/0, index.to(row.device()));
}

[[nodiscard]] bool extract_row_vectors_(
    const torch::Tensor& features,
    const torch::Tensor& mask,
    const data_analytics_options_t& options,
    extracted_sequence_rows_t* out) {
  if (!out) return false;
  *out = extracted_sequence_rows_t{};

  torch::Tensor f;
  torch::Tensor m;
  if (!normalize_feature_and_mask_(features, mask, &f, &m)) {
    return false;
  }
  if (!tensor_is_finite_(m)) {
    return false;
  }

  const std::int64_t B = f.size(0);
  const std::int64_t C = f.size(1);
  const std::int64_t T = f.size(2);
  const std::int64_t D = f.size(3);
  const std::int64_t flat_features = C * T * D;
  const torch::Tensor sampled_index =
      deterministic_feature_subsample_index_(flat_features, options.max_features);
  if (!sampled_index.defined() || sampled_index.dim() != 1 ||
      sampled_index.numel() <= 0) {
    return false;
  }

  out->channels = C;
  out->timesteps = T;
  out->features_per_timestep = D;
  out->flat_feature_count = flat_features;
  out->sampled_feature_count = sampled_index.size(0);

  for (std::int64_t b = 0; b < B; ++b) {
    ++out->sample_count;

    torch::Tensor mask_b = m[b];
    torch::Tensor valid_b = mask_b > 0.0;
    double accepted_timestep_ratio = 0.0;
    try {
      accepted_timestep_ratio = valid_b.to(torch::kFloat64).mean().item<double>();
    } catch (...) {
      accepted_timestep_ratio = 0.0;
    }

    if (!std::isfinite(accepted_timestep_ratio) ||
        accepted_timestep_ratio <= options.mask_epsilon) {
      ++out->skipped_sample_count;
      continue;
    }

    torch::Tensor row = f[b].reshape({flat_features});
    torch::Tensor valid_row =
        valid_b.to(torch::kFloat64)
            .unsqueeze(-1)
            .expand({C, T, D})
            .reshape({flat_features});
    row = sample_feature_vector_(row, sampled_index);
    valid_row = sample_feature_vector_(valid_row, sampled_index);
    if (!row.defined() || row.dim() != 1 || row.numel() <= 0) {
      ++out->skipped_sample_count;
      continue;
    }
    if (!valid_row.defined() || valid_row.dim() != 1 ||
        valid_row.size(0) != row.size(0)) {
      ++out->skipped_sample_count;
      continue;
    }
    row = torch::where(valid_row > 0.0, row, torch::zeros_like(row));
    if (!tensor_is_finite_(row) || !tensor_is_finite_(valid_row)) {
      ++out->skipped_sample_count;
      continue;
    }
    const double sampled_valid_mass = valid_row.sum().item<double>();
    if (!std::isfinite(sampled_valid_mass) || sampled_valid_mass <= kNumericEpsilon) {
      ++out->skipped_sample_count;
      continue;
    }

    if (static_cast<std::int64_t>(out->rows.size()) >= options.max_samples) {
      ++out->skipped_sample_count;
      continue;
    }

    out->rows.push_back(std::move(row));
    out->validity_masks.push_back(std::move(valid_row));
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

[[nodiscard]] bool write_text_file_atomically_(
    const std::string& payload,
    const std::filesystem::path& output_file,
    std::string_view artifact_name,
    std::string* error) {
  if (error) error->clear();

  std::error_code ec;
  const auto parent = output_file.parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent, ec);
    if (ec) {
      if (error) {
        *error = "cannot create " + std::string(artifact_name) +
                 " directory: " + parent.string();
      }
      return false;
    }
  }

  static std::atomic<std::uint64_t> s_temp_counter{0};
  const auto temp_suffix =
      std::to_string(static_cast<std::uint64_t>(
                         std::chrono::steady_clock::now()
                             .time_since_epoch()
                             .count())) +
      "." + std::to_string(s_temp_counter.fetch_add(1, std::memory_order_relaxed));
  const std::filesystem::path temp_file =
      output_file.string() + ".tmp." + temp_suffix;

  {
    std::ofstream out(temp_file, std::ios::binary | std::ios::trunc);
    if (!out) {
      if (error) {
        *error = "cannot open temporary " + std::string(artifact_name) +
                 " file: " + temp_file.string();
      }
      return false;
    }
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    out.close();
    if (!out) {
      std::error_code remove_ec;
      std::filesystem::remove(temp_file, remove_ec);
      if (error) {
        *error = "cannot write temporary " + std::string(artifact_name) +
                 " file: " + temp_file.string();
      }
      return false;
    }
  }

  std::filesystem::rename(temp_file, output_file, ec);
  if (ec) {
    std::error_code remove_ec;
    std::filesystem::remove(temp_file, remove_ec);
    if (error) {
      *error = "cannot replace " + std::string(artifact_name) +
               " file: " + output_file.string();
    }
    return false;
  }
  return true;
}

}  // namespace

sequence_analytics_accumulator_t::sequence_analytics_accumulator_t(
    data_analytics_options_t options)
    : options_(normalize_options_(options)) {}

void sequence_analytics_accumulator_t::reset() {
  rows_.clear();
  row_validity_masks_.clear();
  sample_count_ = 0;
  skipped_sample_count_ = 0;
  sequence_channels_ = 0;
  sequence_timesteps_ = 0;
  sequence_features_per_timestep_ = 0;
  sequence_flat_feature_count_ = 0;
  sequence_sampled_feature_count_ = 0;
}

bool sequence_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  extracted_sequence_rows_t extracted{};
  if (!extract_row_vectors_(features, mask, options_, &extracted)) {
    ++sample_count_;
    ++skipped_sample_count_;
    return false;
  }

  sample_count_ += extracted.sample_count;
  skipped_sample_count_ += extracted.skipped_sample_count;

  const bool geometry_locked = sequence_flat_feature_count_ > 0;
  const bool geometry_mismatch =
      geometry_locked &&
      (sequence_channels_ != extracted.channels ||
       sequence_timesteps_ != extracted.timesteps ||
       sequence_features_per_timestep_ != extracted.features_per_timestep ||
       sequence_flat_feature_count_ != extracted.flat_feature_count ||
       sequence_sampled_feature_count_ != extracted.sampled_feature_count);
  if (geometry_mismatch) {
    skipped_sample_count_ +=
        static_cast<std::uint64_t>(extracted.rows.size());
    return false;
  }

  if (!geometry_locked && !extracted.rows.empty()) {
    sequence_channels_ = extracted.channels;
    sequence_timesteps_ = extracted.timesteps;
    sequence_features_per_timestep_ = extracted.features_per_timestep;
    sequence_flat_feature_count_ = extracted.flat_feature_count;
    sequence_sampled_feature_count_ = extracted.sampled_feature_count;
  }

  if (!extracted.rows.empty()) {
    rows_.insert(
        rows_.end(),
        std::make_move_iterator(extracted.rows.begin()),
        std::make_move_iterator(extracted.rows.end()));
    row_validity_masks_.insert(
        row_validity_masks_.end(),
        std::make_move_iterator(extracted.validity_masks.begin()),
        std::make_move_iterator(extracted.validity_masks.end()));
  }

  return true;
}

sequence_analytics_report_t sequence_analytics_accumulator_t::summarize()
    const {
  sequence_analytics_report_t out{};
  out.sample_count = sample_count_;
  out.skipped_sample_count = skipped_sample_count_;
  out.valid_sample_count = static_cast<std::uint64_t>(rows_.size());
  out.sequence_channels = sequence_channels_;
  out.sequence_timesteps = sequence_timesteps_;
  out.sequence_features_per_timestep = sequence_features_per_timestep_;
  out.sequence_flat_feature_count = sequence_flat_feature_count_;
  out.sequence_effective_feature_count = 0;

  if (rows_.empty()) return out;
  if (rows_.size() != row_validity_masks_.size()) return out;

  torch::Tensor X;
  torch::Tensor V;
  try {
    X = torch::stack(rows_, /*dim=*/0);  // [N,F]
    V = torch::stack(row_validity_masks_, /*dim=*/0);  // [N,F]
  } catch (...) {
    return out;
  }

  if (!X.defined() || X.dim() != 2 || X.size(0) <= 0 || X.size(1) <= 0) {
    return out;
  }
  if (!V.defined() || V.dim() != 2 || V.sizes() != X.sizes()) {
    return out;
  }

  try {
    V = V.clamp(0.0, 1.0);
    torch::Tensor valid_mass = V.sum(/*dim=*/0);  // [F]
    const torch::Tensor active = valid_mass > kNumericEpsilon;
    const std::int64_t active_feature_count =
        active.sum().item<std::int64_t>();
    out.sequence_effective_feature_count = active_feature_count;
    if (active_feature_count <= 0) return out;

    const torch::Tensor active_index =
        torch::nonzero(active).reshape({active_feature_count});
    X = X.index_select(/*dim=*/1, active_index);
    V = V.index_select(/*dim=*/1, active_index);
    valid_mass = valid_mass.index_select(/*dim=*/0, active_index);

    const torch::Tensor mean = (X * V).sum(/*dim=*/0) / valid_mass;
    const torch::Tensor centered = (X - mean) * V;
    const torch::Tensor var =
        (centered * centered).sum(/*dim=*/0) / valid_mass;
    const torch::Tensor stdev =
        torch::sqrt(var.clamp_min(options_.standardize_epsilon));
    const torch::Tensor Z = centered / stdev;

    torch::Tensor support = V.transpose(0, 1).mm(V);
    support = support.clamp_min(1.0);
    const torch::Tensor cov = Z.transpose(0, 1).mm(Z) / support;

    torch::Tensor eigvals = at::linalg_eigvalsh(cov, "L");
    eigvals = eigvals.clamp_min(0.0);

    const double trace = eigvals.sum().item<double>();
    out.sequence_cov_trace = std::isfinite(trace) ? trace : 0.0;

    if (!std::isfinite(trace) || trace <= kNumericEpsilon) return out;

    const torch::Tensor probs = eigvals / trace;
    const torch::Tensor probs_safe = probs.clamp_min(kNumericEpsilon);
    const double entropy =
        (-probs * torch::log(probs_safe)).sum().item<double>();

    if (std::isfinite(entropy)) {
      out.sequence_entropic_load = std::exp(entropy);
    }

    out.sequence_nonzero_eigen_count = static_cast<std::uint64_t>(
        (eigvals > options_.standardize_epsilon).sum().item<std::int64_t>());
  } catch (...) {
    return out;
  }

  return out;
}

data_source_analytics_accumulator_t::data_source_analytics_accumulator_t(
    data_analytics_options_t options)
    : core_(options) {}

void data_source_analytics_accumulator_t::reset() { core_.reset(); }

bool data_source_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  return core_.ingest(features, mask);
}

data_source_analytics_report_t data_source_analytics_accumulator_t::summarize()
    const {
  return make_data_source_analytics_report(core_.summarize());
}

sequence_symbolic_analytics_accumulator_t::sequence_symbolic_analytics_accumulator_t(
    std::vector<sequence_symbolic_stream_descriptor_t> stream_descriptors)
    : descriptors_(std::move(stream_descriptors)) {
  reset();
}

void sequence_symbolic_analytics_accumulator_t::reset() {
  stream_series_.assign(descriptors_.size(), {});
}

bool sequence_symbolic_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  torch::Tensor f;
  torch::Tensor m;
  if (!normalize_feature_and_mask_(features, mask, &f, &m)) {
    return false;
  }
  if (!tensor_is_finite_(f) || !tensor_is_finite_(m)) {
    return false;
  }
  if (f.size(1) != static_cast<std::int64_t>(descriptors_.size())) {
    return false;
  }

  const std::int64_t B = f.size(0);
  const std::int64_t C = f.size(1);
  const std::int64_t T = f.size(2);
  const std::int64_t D = f.size(3);
  if (T <= 0 || D <= 0) return false;

  for (std::int64_t b = 0; b < B; ++b) {
    for (std::int64_t c = 0; c < C; ++c) {
      const auto& descriptor = descriptors_[static_cast<std::size_t>(c)];
      if (descriptor.anchor_feature_index < 0 ||
          descriptor.anchor_feature_index >= D) {
        continue;
      }

      std::int64_t anchor_timestep = -1;
      for (std::int64_t t = T - 1; t >= 0; --t) {
        const double mask_value = m.index({b, c, t}).item<double>();
        if (std::isfinite(mask_value) && mask_value > 0.0) {
          anchor_timestep = t;
          break;
        }
      }
      if (anchor_timestep < 0) continue;

      // Symbolic heuristics operate on the anchor series induced by ingest order.
      const double anchor_value =
          f.index({b, c, anchor_timestep, descriptor.anchor_feature_index})
              .item<double>();
      if (!std::isfinite(anchor_value)) continue;

      stream_series_[static_cast<std::size_t>(c)].push_back(anchor_value);
    }
  }
  return true;
}

sequence_symbolic_analytics_report_t
sequence_symbolic_analytics_accumulator_t::summarize() const {
  sequence_symbolic_analytics_report_t out{};
  out.stream_count = static_cast<std::uint64_t>(descriptors_.size());
  out.reported_stream_count = out.stream_count;
  out.omitted_stream_count = 0;
  out.stream_report_reduced = false;
  out.streams.reserve(descriptors_.size());

  double lz_sum = 0.0;
  double info_sum = 0.0;
  double compression_sum = 0.0;
  double autocorr_decay_sum = 0.0;
  double spectrum_entropy_sum = 0.0;
  double hurst_sum = 0.0;
  bool have_eligible = false;

  for (std::size_t i = 0; i < descriptors_.size(); ++i) {
    const auto& descriptor = descriptors_[i];
    const auto& series = stream_series_[i];

    sequence_symbolic_stream_report_t stream{};
    stream.label = descriptor.label;
    stream.stream_family = descriptor.stream_family;
    stream.anchor_feature = descriptor.anchor_feature;
    stream.feature_names = descriptor.feature_names;
    stream.valid_count = static_cast<std::uint64_t>(series.size());

    const std::vector<std::uint8_t> tokens = symbolize_ternary_quantiles_(series);
    stream.observed_symbol_count = observed_symbol_count_(tokens);
    stream.eligible = stream.valid_count >= kSymbolicMinEligibleSamples;

    if (stream.eligible) {
      stream.lz76_complexity = lz76_complexity_(tokens);
      stream.lz76_normalized = clamp_nonneg(
          normalized_lz76_complexity_(stream.lz76_complexity, tokens.size()));
      stream.entropy_rate_bits =
          clamp_nonneg(entropy_rate_bits_from_bigrams_(tokens));
      const double density =
          stream.entropy_rate_bits /
          std::log2(static_cast<double>(kSymbolicAlphabetSize));
      if (std::isfinite(density)) {
        stream.information_density = std::clamp(density, 0.0, 1.0);
      }
      stream.compression_ratio = clamp_nonneg(compression_ratio_lzw_(tokens));
      stream.autocorrelation_decay_lag =
          clamp_nonneg(autocorrelation_decay_lag_(series));
      stream.power_spectrum_entropy =
          std::clamp(power_spectrum_entropy_(series), 0.0, 1.0);
      stream.hurst_exponent =
          std::clamp(hurst_exponent_rs_(series), 0.0, 1.0);

      lz_sum += stream.lz76_normalized;
      info_sum += stream.information_density;
      compression_sum += stream.compression_ratio;
      autocorr_decay_sum += stream.autocorrelation_decay_lag;
      spectrum_entropy_sum += stream.power_spectrum_entropy;
      hurst_sum += stream.hurst_exponent;
      ++out.eligible_stream_count;

      if (!have_eligible) {
        out.lz76_normalized_min = stream.lz76_normalized;
        out.lz76_normalized_max = stream.lz76_normalized;
        out.lz76_normalized_min_label = stream.label;
        out.lz76_normalized_max_label = stream.label;
        out.information_density_min = stream.information_density;
        out.information_density_max = stream.information_density;
        out.information_density_min_label = stream.label;
        out.information_density_max_label = stream.label;
        out.compression_ratio_min = stream.compression_ratio;
        out.compression_ratio_max = stream.compression_ratio;
        out.compression_ratio_min_label = stream.label;
        out.compression_ratio_max_label = stream.label;
        out.autocorrelation_decay_lag_min = stream.autocorrelation_decay_lag;
        out.autocorrelation_decay_lag_max = stream.autocorrelation_decay_lag;
        out.autocorrelation_decay_lag_min_label = stream.label;
        out.autocorrelation_decay_lag_max_label = stream.label;
        out.power_spectrum_entropy_min = stream.power_spectrum_entropy;
        out.power_spectrum_entropy_max = stream.power_spectrum_entropy;
        out.power_spectrum_entropy_min_label = stream.label;
        out.power_spectrum_entropy_max_label = stream.label;
        out.hurst_exponent_min = stream.hurst_exponent;
        out.hurst_exponent_max = stream.hurst_exponent;
        out.hurst_exponent_min_label = stream.label;
        out.hurst_exponent_max_label = stream.label;
        have_eligible = true;
      } else {
        if (stream.lz76_normalized < out.lz76_normalized_min) {
          out.lz76_normalized_min = stream.lz76_normalized;
          out.lz76_normalized_min_label = stream.label;
        }
        if (stream.lz76_normalized > out.lz76_normalized_max) {
          out.lz76_normalized_max = stream.lz76_normalized;
          out.lz76_normalized_max_label = stream.label;
        }
        if (stream.information_density < out.information_density_min) {
          out.information_density_min = stream.information_density;
          out.information_density_min_label = stream.label;
        }
        if (stream.information_density > out.information_density_max) {
          out.information_density_max = stream.information_density;
          out.information_density_max_label = stream.label;
        }
        if (stream.compression_ratio < out.compression_ratio_min) {
          out.compression_ratio_min = stream.compression_ratio;
          out.compression_ratio_min_label = stream.label;
        }
        if (stream.compression_ratio > out.compression_ratio_max) {
          out.compression_ratio_max = stream.compression_ratio;
          out.compression_ratio_max_label = stream.label;
        }
        if (stream.autocorrelation_decay_lag <
            out.autocorrelation_decay_lag_min) {
          out.autocorrelation_decay_lag_min = stream.autocorrelation_decay_lag;
          out.autocorrelation_decay_lag_min_label = stream.label;
        }
        if (stream.autocorrelation_decay_lag >
            out.autocorrelation_decay_lag_max) {
          out.autocorrelation_decay_lag_max = stream.autocorrelation_decay_lag;
          out.autocorrelation_decay_lag_max_label = stream.label;
        }
        if (stream.power_spectrum_entropy < out.power_spectrum_entropy_min) {
          out.power_spectrum_entropy_min = stream.power_spectrum_entropy;
          out.power_spectrum_entropy_min_label = stream.label;
        }
        if (stream.power_spectrum_entropy > out.power_spectrum_entropy_max) {
          out.power_spectrum_entropy_max = stream.power_spectrum_entropy;
          out.power_spectrum_entropy_max_label = stream.label;
        }
        if (stream.hurst_exponent < out.hurst_exponent_min) {
          out.hurst_exponent_min = stream.hurst_exponent;
          out.hurst_exponent_min_label = stream.label;
        }
        if (stream.hurst_exponent > out.hurst_exponent_max) {
          out.hurst_exponent_max = stream.hurst_exponent;
          out.hurst_exponent_max_label = stream.label;
        }
      }
    }

    out.streams.push_back(std::move(stream));
  }

  if (out.eligible_stream_count > 0) {
    const double denom = static_cast<double>(out.eligible_stream_count);
    out.lz76_normalized_mean = lz_sum / denom;
    out.information_density_mean = info_sum / denom;
    out.compression_ratio_mean = compression_sum / denom;
    out.autocorrelation_decay_lag_mean = autocorr_decay_sum / denom;
    out.power_spectrum_entropy_mean = spectrum_entropy_sum / denom;
    out.hurst_exponent_mean = hurst_sum / denom;
  }

  return out;
}

data_symbolic_analytics_accumulator_t::data_symbolic_analytics_accumulator_t(
    std::vector<data_symbolic_channel_descriptor_t> channel_descriptors)
    : descriptors_(std::move(channel_descriptors)) {
  std::vector<sequence_symbolic_stream_descriptor_t> sequence_descriptors;
  sequence_descriptors.reserve(descriptors_.size());
  for (const auto& descriptor : descriptors_) {
    sequence_descriptors.push_back(
        make_sequence_symbolic_stream_descriptor(descriptor));
  }
  core_ = sequence_symbolic_analytics_accumulator_t(
      std::move(sequence_descriptors));
}

void data_symbolic_analytics_accumulator_t::reset() { core_.reset(); }

bool data_symbolic_analytics_accumulator_t::ingest(
    const torch::Tensor& features,
    const torch::Tensor& mask) {
  return core_.ingest(features, mask);
}

data_symbolic_analytics_report_t data_symbolic_analytics_accumulator_t::summarize()
    const {
  return make_data_symbolic_analytics_report(core_.summarize());
}

sequence_analytics_report_t make_sequence_analytics_report(
    const data_source_analytics_report_t& report) {
  sequence_analytics_report_t out{};
  out.sample_count = report.sample_count;
  out.valid_sample_count = report.valid_sample_count;
  out.skipped_sample_count = report.skipped_sample_count;
  out.sequence_channels = report.source_channels;
  out.sequence_timesteps = report.source_timesteps;
  out.sequence_features_per_timestep = report.source_features_per_timestep;
  out.sequence_flat_feature_count = report.source_flat_feature_count;
  out.sequence_effective_feature_count = report.source_effective_feature_count;
  out.sequence_entropic_load = report.source_entropic_load;
  out.sequence_cov_trace = report.source_cov_trace;
  out.sequence_nonzero_eigen_count = report.source_nonzero_eigen_count;
  return out;
}

