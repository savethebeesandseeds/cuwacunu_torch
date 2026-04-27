data_source_analytics_report_t make_data_source_analytics_report(
    const sequence_analytics_report_t& report) {
  data_source_analytics_report_t out{};
  out.sample_count = report.sample_count;
  out.valid_sample_count = report.valid_sample_count;
  out.skipped_sample_count = report.skipped_sample_count;
  out.source_channels = report.sequence_channels;
  out.source_timesteps = report.sequence_timesteps;
  out.source_features_per_timestep = report.sequence_features_per_timestep;
  out.source_flat_feature_count = report.sequence_flat_feature_count;
  out.source_effective_feature_count = report.sequence_effective_feature_count;
  out.source_entropic_load = report.sequence_entropic_load;
  out.source_cov_trace = report.sequence_cov_trace;
  out.source_nonzero_eigen_count = report.sequence_nonzero_eigen_count;
  return out;
}

sequence_symbolic_stream_descriptor_t make_sequence_symbolic_stream_descriptor(
    const data_symbolic_channel_descriptor_t& descriptor) {
  sequence_symbolic_stream_descriptor_t out{};
  out.label = descriptor.label;
  out.stream_family = descriptor.record_type;
  out.anchor_feature = descriptor.anchor_feature;
  out.feature_names = descriptor.feature_names;
  out.anchor_feature_index = descriptor.anchor_feature_index;
  return out;
}

data_symbolic_channel_descriptor_t make_data_symbolic_channel_descriptor(
    const sequence_symbolic_stream_descriptor_t& descriptor) {
  data_symbolic_channel_descriptor_t out{};
  out.label = descriptor.label;
  out.record_type = descriptor.stream_family;
  out.anchor_feature = descriptor.anchor_feature;
  out.feature_names = descriptor.feature_names;
  out.anchor_feature_index = descriptor.anchor_feature_index;
  return out;
}

sequence_symbolic_analytics_report_t make_sequence_symbolic_analytics_report(
    const data_symbolic_analytics_report_t& report) {
  sequence_symbolic_analytics_report_t out{};
  out.stream_count = report.channel_count;
  out.eligible_stream_count = report.eligible_channel_count;
  out.reported_stream_count = report.channel_count;
  out.omitted_stream_count = 0;
  out.stream_report_reduced = false;
  out.lz76_normalized_mean = report.lz76_normalized_mean;
  out.lz76_normalized_min = report.lz76_normalized_min;
  out.lz76_normalized_min_label = report.lz76_normalized_min_label;
  out.lz76_normalized_max = report.lz76_normalized_max;
  out.lz76_normalized_max_label = report.lz76_normalized_max_label;
  out.information_density_mean = report.information_density_mean;
  out.information_density_min = report.information_density_min;
  out.information_density_min_label = report.information_density_min_label;
  out.information_density_max = report.information_density_max;
  out.information_density_max_label = report.information_density_max_label;
  out.compression_ratio_mean = report.compression_ratio_mean;
  out.compression_ratio_min = report.compression_ratio_min;
  out.compression_ratio_min_label = report.compression_ratio_min_label;
  out.compression_ratio_max = report.compression_ratio_max;
  out.compression_ratio_max_label = report.compression_ratio_max_label;
  out.autocorrelation_decay_lag_mean = report.autocorrelation_decay_lag_mean;
  out.autocorrelation_decay_lag_min = report.autocorrelation_decay_lag_min;
  out.autocorrelation_decay_lag_min_label =
      report.autocorrelation_decay_lag_min_label;
  out.autocorrelation_decay_lag_max = report.autocorrelation_decay_lag_max;
  out.autocorrelation_decay_lag_max_label =
      report.autocorrelation_decay_lag_max_label;
  out.power_spectrum_entropy_mean = report.power_spectrum_entropy_mean;
  out.power_spectrum_entropy_min = report.power_spectrum_entropy_min;
  out.power_spectrum_entropy_min_label =
      report.power_spectrum_entropy_min_label;
  out.power_spectrum_entropy_max = report.power_spectrum_entropy_max;
  out.power_spectrum_entropy_max_label =
      report.power_spectrum_entropy_max_label;
  out.hurst_exponent_mean = report.hurst_exponent_mean;
  out.hurst_exponent_min = report.hurst_exponent_min;
  out.hurst_exponent_min_label = report.hurst_exponent_min_label;
  out.hurst_exponent_max = report.hurst_exponent_max;
  out.hurst_exponent_max_label = report.hurst_exponent_max_label;
  out.streams.reserve(report.channels.size());
  for (const auto& channel : report.channels) {
    sequence_symbolic_stream_report_t stream{};
    stream.label = channel.label;
    stream.stream_family = channel.record_type;
    stream.anchor_feature = channel.anchor_feature;
    stream.feature_names = channel.feature_names;
    stream.valid_count = channel.valid_count;
    stream.observed_symbol_count = channel.observed_symbol_count;
    stream.eligible = channel.eligible;
    stream.lz76_complexity = channel.lz76_complexity;
    stream.lz76_normalized = channel.lz76_normalized;
    stream.entropy_rate_bits = channel.entropy_rate_bits;
    stream.information_density = channel.information_density;
    stream.compression_ratio = channel.compression_ratio;
    stream.autocorrelation_decay_lag = channel.autocorrelation_decay_lag;
    stream.power_spectrum_entropy = channel.power_spectrum_entropy;
    stream.hurst_exponent = channel.hurst_exponent;
    out.streams.push_back(std::move(stream));
  }
  return out;
}

sequence_symbolic_analytics_report_t compact_sequence_symbolic_analytics_report(
    const sequence_symbolic_analytics_report_t& report,
    sequence_symbolic_report_compaction_options_t options) {
  auto normalized = normalize_sequence_symbolic_report_compaction_options_(options);
  if (report.streams.empty()) return report;
  if (report.stream_report_reduced) return report;
  if (normalized.max_reported_streams <= 0) return report;
  if (report.stream_count <=
      static_cast<std::uint64_t>(normalized.reduce_when_stream_count_exceeds)) {
    return report;
  }
  if (report.streams.size() <=
      static_cast<std::size_t>(normalized.max_reported_streams)) {
    return report;
  }

  sequence_symbolic_analytics_report_t out = report;
  const std::size_t max_reported =
      std::min<std::size_t>(
          static_cast<std::size_t>(normalized.max_reported_streams),
          report.streams.size());

  std::vector<std::size_t> selected;
  selected.reserve(max_reported);
  add_stream_index_for_label_(report, report.lz76_normalized_min_label, &selected);
  add_stream_index_for_label_(report, report.lz76_normalized_max_label, &selected);
  add_stream_index_for_label_(report, report.information_density_min_label, &selected);
  add_stream_index_for_label_(report, report.information_density_max_label, &selected);
  add_stream_index_for_label_(report, report.compression_ratio_min_label, &selected);
  add_stream_index_for_label_(report, report.compression_ratio_max_label, &selected);
  add_stream_index_for_label_(
      report, report.autocorrelation_decay_lag_min_label, &selected);
  add_stream_index_for_label_(
      report, report.autocorrelation_decay_lag_max_label, &selected);
  add_stream_index_for_label_(
      report, report.power_spectrum_entropy_min_label, &selected);
  add_stream_index_for_label_(
      report, report.power_spectrum_entropy_max_label, &selected);
  add_stream_index_for_label_(report, report.hurst_exponent_min_label, &selected);
  add_stream_index_for_label_(report, report.hurst_exponent_max_label, &selected);

  if (selected.size() > max_reported) {
    selected.resize(max_reported);
  }
  fill_remaining_stream_indices_evenly_(report, max_reported, &selected);
  if (selected.size() > max_reported) {
    selected.resize(max_reported);
  }
  std::sort(selected.begin(), selected.end());
  selected.erase(std::unique(selected.begin(), selected.end()), selected.end());

  if (selected.size() < max_reported) {
    fill_remaining_stream_indices_evenly_(report, max_reported, &selected);
    std::sort(selected.begin(), selected.end());
    selected.erase(std::unique(selected.begin(), selected.end()), selected.end());
  }

  out.streams.clear();
  out.streams.reserve(selected.size());
  for (std::size_t index : selected) {
    out.streams.push_back(report.streams[index]);
  }
  out.reported_stream_count = static_cast<std::uint64_t>(out.streams.size());
  out.omitted_stream_count =
      (out.stream_count > out.reported_stream_count)
          ? (out.stream_count - out.reported_stream_count)
          : 0;
  out.stream_report_reduced = out.omitted_stream_count > 0;
  if (out.stream_report_reduced) {
    out.stream_report_reduction_mode = "extrema_plus_even_coverage";
  } else {
    out.stream_report_reduction_mode.clear();
  }
  return out;
}

data_symbolic_analytics_report_t make_data_symbolic_analytics_report(
    const sequence_symbolic_analytics_report_t& report) {
  data_symbolic_analytics_report_t out{};
  out.channel_count = report.stream_count;
  out.eligible_channel_count = report.eligible_stream_count;
  out.lz76_normalized_mean = report.lz76_normalized_mean;
  out.lz76_normalized_min = report.lz76_normalized_min;
  out.lz76_normalized_min_label = report.lz76_normalized_min_label;
  out.lz76_normalized_max = report.lz76_normalized_max;
  out.lz76_normalized_max_label = report.lz76_normalized_max_label;
  out.information_density_mean = report.information_density_mean;
  out.information_density_min = report.information_density_min;
  out.information_density_min_label = report.information_density_min_label;
  out.information_density_max = report.information_density_max;
  out.information_density_max_label = report.information_density_max_label;
  out.compression_ratio_mean = report.compression_ratio_mean;
  out.compression_ratio_min = report.compression_ratio_min;
  out.compression_ratio_min_label = report.compression_ratio_min_label;
  out.compression_ratio_max = report.compression_ratio_max;
  out.compression_ratio_max_label = report.compression_ratio_max_label;
  out.autocorrelation_decay_lag_mean = report.autocorrelation_decay_lag_mean;
  out.autocorrelation_decay_lag_min = report.autocorrelation_decay_lag_min;
  out.autocorrelation_decay_lag_min_label =
      report.autocorrelation_decay_lag_min_label;
  out.autocorrelation_decay_lag_max = report.autocorrelation_decay_lag_max;
  out.autocorrelation_decay_lag_max_label =
      report.autocorrelation_decay_lag_max_label;
  out.power_spectrum_entropy_mean = report.power_spectrum_entropy_mean;
  out.power_spectrum_entropy_min = report.power_spectrum_entropy_min;
  out.power_spectrum_entropy_min_label =
      report.power_spectrum_entropy_min_label;
  out.power_spectrum_entropy_max = report.power_spectrum_entropy_max;
  out.power_spectrum_entropy_max_label =
      report.power_spectrum_entropy_max_label;
  out.hurst_exponent_mean = report.hurst_exponent_mean;
  out.hurst_exponent_min = report.hurst_exponent_min;
  out.hurst_exponent_min_label = report.hurst_exponent_min_label;
  out.hurst_exponent_max = report.hurst_exponent_max;
  out.hurst_exponent_max_label = report.hurst_exponent_max_label;
  out.channels.reserve(report.streams.size());
  for (const auto& stream : report.streams) {
    data_symbolic_channel_report_t channel{};
    channel.label = stream.label;
    channel.record_type = stream.stream_family;
    channel.anchor_feature = stream.anchor_feature;
    channel.feature_names = stream.feature_names;
    channel.valid_count = stream.valid_count;
    channel.observed_symbol_count = stream.observed_symbol_count;
    channel.eligible = stream.eligible;
    channel.lz76_complexity = stream.lz76_complexity;
    channel.lz76_normalized = stream.lz76_normalized;
    channel.entropy_rate_bits = stream.entropy_rate_bits;
    channel.information_density = stream.information_density;
    channel.compression_ratio = stream.compression_ratio;
    channel.autocorrelation_decay_lag = stream.autocorrelation_decay_lag;
    channel.power_spectrum_entropy = stream.power_spectrum_entropy;
    channel.hurst_exponent = stream.hurst_exponent;
    out.channels.push_back(std::move(channel));
  }
  return out;
}

const std::vector<std::string>& data_feature_names_for_record_type(
    std::string_view record_type) {
  if (record_type == "kline") return kline_feature_names_();
  if (record_type == "trade") return trade_feature_names_();
  if (record_type == "basic") return basic_feature_names_();
  return empty_feature_names_();
}

std::string_view data_symbolic_anchor_feature_name_for_record_type(
    std::string_view record_type) {
  if (record_type == "kline") return "close_price";
  if (record_type == "trade") return "price";
  if (record_type == "basic") return "value";
  return {};
}

std::int64_t data_symbolic_anchor_feature_index_for_record_type(
    std::string_view record_type) {
  if (record_type == "kline") return 3;
  if (record_type == "trade") return 0;
  if (record_type == "basic") return 0;
  return -1;
}

