void emit_periodic_summary_(const Wave &wave, Emitter &out, bool force) const {
  const bool cadence_hit =
      (cfg_.summary_every_steps > 0) && (epoch_.seen > 0) &&
      ((epoch_.seen % static_cast<std::uint64_t>(cfg_.summary_every_steps)) ==
       0);
  if (!force && !cadence_hit)
    return;

  const std::uint64_t invalid =
      (epoch_.seen >= epoch_.accepted) ? (epoch_.seen - epoch_.accepted) : 0;

  std::ostringstream oss;
  oss << "transfer_matrix_eval.summary"
      << " seen=" << epoch_.seen << " accepted=" << epoch_.accepted
      << " buffered_batches=" << epoch_batches_.size() << " invalid=" << invalid
      << " train_batches_attempted=" << epoch_.train_batches_attempted
      << " train_steps=" << epoch_.train_steps
      << " train_ingested_positions=" << epoch_.train_ingested_positions
      << " train_ingested_support=" << epoch_.train_ingested_support
      << " null_cargo=" << epoch_.null_cargo
      << " invalid_cargo=" << epoch_.invalid_cargo
      << " missing_encoding=" << epoch_.missing_encoding
      << " non_finite_encoding=" << epoch_.non_finite_encoding
      << " temporal_overlap=" << epoch_.temporal_overlap
      << " shape_mismatch=" << epoch_.shape_mismatch
      << " optimizer_failures=" << epoch_.optimizer_failures;
  emit_meta_(wave, out, oss.str());
}

[[nodiscard]] epoch_report_metrics_t collect_epoch_report_metrics_() const {
  epoch_report_metrics_t m{};
  m.invalid =
      (epoch_.seen >= epoch_.accepted) ? (epoch_.seen - epoch_.accepted) : 0;
  m.buffered_batches = static_cast<std::uint64_t>(epoch_batches_.size());
  m.matrix = epoch_matrix_;
  return m;
}

[[nodiscard]] static double sorted_quantile_1d_(const torch::Tensor &sorted,
                                                double q) {
  if (!sorted.defined() || sorted.dim() != 1 || sorted.numel() <= 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const std::int64_t n = sorted.size(0);
  if (n <= 0)
    return std::numeric_limits<double>::quiet_NaN();
  if (n == 1)
    return sorted.select(/*dim=*/0, /*index=*/0).item<double>();

  const double qq = std::clamp(q, 0.0, 1.0);
  const double pos = qq * static_cast<double>(n - 1);
  const auto lo = static_cast<std::int64_t>(std::floor(pos));
  const auto hi = static_cast<std::int64_t>(std::ceil(pos));
  const double lo_v = sorted.select(/*dim=*/0, /*index=*/lo).item<double>();
  const double hi_v = sorted.select(/*dim=*/0, /*index=*/hi).item<double>();
  if (lo == hi)
    return lo_v;
  const double w = pos - static_cast<double>(lo);
  return lo_v + (hi_v - lo_v) * w;
}

void record_prequential_row_(std::string_view method, std::size_t block_index,
                             double prefix_support, double eval_support,
                             double model_bits, double null_bits) {
  ++epoch_prequential_rows_total_;
  if (epoch_prequential_rows_.size() >= kMaxPersistedPrequentialRows_) {
    epoch_prequential_rows_truncated_ = true;
    return;
  }
  prequential_row_t row{};
  row.method = std::string(method);
  row.block_index = static_cast<std::uint64_t>(block_index);
  row.prefix_support = prefix_support;
  row.eval_support = eval_support;
  row.model_bits = model_bits;
  row.null_bits = null_bits;
  if (std::isfinite(null_bits) && std::isfinite(model_bits)) {
    row.skill_bits = null_bits - model_bits;
  }
  epoch_prequential_rows_.push_back(std::move(row));
}

[[nodiscard]] bool
summarize_epoch_activation_report_(activation_report_t *out) const {
  if (!out)
    return false;
  *out = activation_report_t{};
  out->batch_count = static_cast<std::uint64_t>(epoch_batches_.size());
  out->encoding_dims = static_cast<std::uint64_t>(
      std::max<std::int64_t>(0, model_.encoding_dims));

  std::vector<torch::Tensor> encodings{};
  encodings.reserve(epoch_batches_.size());
  for (const auto &batch : epoch_batches_) {
    if (!batch.encoding.defined() || batch.encoding.numel() == 0 ||
        batch.encoding.dim() != 2) {
      continue;
    }
    encodings.push_back(batch.encoding.to(torch::kCPU, torch::kFloat64));
  }
  if (encodings.empty())
    return false;

  torch::Tensor x{};
  try {
    x = torch::cat(encodings, 0);
  } catch (...) {
    return false;
  }
  if (!x.defined() || x.dim() != 2 || x.numel() == 0)
    return false;

  out->sample_count = static_cast<std::uint64_t>(x.size(0));
  out->encoding_dims = static_cast<std::uint64_t>(x.size(1));

  auto finite = torch::isfinite(x);
  out->finite_ratio = finite.to(torch::kFloat64).mean().item<double>();
  x = torch::where(finite, x, torch::zeros_like(x));

  auto abs_x = torch::abs(x);
  auto centered_all = x - x.mean();
  out->mean = x.mean().item<double>();
  out->stddev = std::sqrt((centered_all * centered_all).mean().item<double>());
  out->l1_mean_abs = abs_x.mean().item<double>();
  out->l2_rms = std::sqrt((x * x).mean().item<double>());

  auto abs_flat = abs_x.flatten();
  if (abs_flat.defined() && abs_flat.numel() > 0) {
    auto abs_sorted =
        std::get<0>(abs_flat.sort(/*dim=*/0, /*descending=*/false));
    out->abs_p50 = sorted_quantile_1d_(abs_sorted, 0.50);
    out->abs_p90 = sorted_quantile_1d_(abs_sorted, 0.90);
    out->abs_p99 = sorted_quantile_1d_(abs_sorted, 0.99);
  }

  auto per_dim_centered = x - x.mean(/*dim=*/0, /*keepdim=*/true);
  auto per_dim_std =
      (per_dim_centered * per_dim_centered).mean(/*dim=*/0).sqrt();
  out->per_dim_std_mean = per_dim_std.mean().item<double>();
  out->per_dim_std_min = per_dim_std.min().item<double>();
  out->per_dim_std_max = per_dim_std.max().item<double>();
  out->dead_dim_ratio = per_dim_std.le(out->dead_dim_epsilon)
                            .to(torch::kFloat64)
                            .mean()
                            .item<double>();

  if (x.size(0) >= 2 && x.size(1) > 0) {
    auto centered = x - x.mean(/*dim=*/0, /*keepdim=*/true);
    const double denom =
        std::max<double>(1.0, static_cast<double>(x.size(0) - 1));
    auto cov = centered.transpose(0, 1).matmul(centered) / denom;
    out->cov_trace = cov.trace().item<double>();

    try {
      auto svd =
          at::linalg_svd(centered, /*full_matrices=*/false, c10::nullopt);
      auto s = std::get<1>(svd).to(torch::kFloat64).contiguous();
      if (s.defined() && s.numel() > 0) {
        auto eig = (s * s) / denom;
        out->cov_top_eigenvalue = eig.max().item<double>();
        const double min_eig = eig.min().item<double>();
        if (std::isfinite(out->cov_top_eigenvalue) && std::isfinite(min_eig)) {
          out->cov_condition_proxy =
              out->cov_top_eigenvalue / std::max(1e-12, min_eig);
        }
        const double eig_sum = eig.sum().item<double>();
        if (std::isfinite(eig_sum) && eig_sum > 0.0) {
          auto p = eig / eig_sum;
          auto p_clamped = p.clamp_min(1e-18);
          const double entropy = (-(p * p_clamped.log()).sum()).item<double>();
          if (std::isfinite(entropy)) {
            out->cov_effective_rank = std::exp(entropy);
          }
        }
      }
    } catch (...) {
      // Keep activation report best-effort even when SVD fails.
    }
  }

  out->valid = true;
  return true;
}

static void append_lls_str_(runtime_lls_document_t &out, std::string_view key,
                            std::string_view value) {
  out.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_string_entry(
          std::string(key), std::string(value)));
}

static void append_lls_u64_(runtime_lls_document_t &out, std::string_view key,
                            std::uint64_t value,
                            std::string_view declared_domain = "(0,+inf)") {
  out.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_uint_entry(
          std::string(key), value, std::string(declared_domain)));
}

static void
append_lls_double_(runtime_lls_document_t &out, std::string_view key,
                   double value,
                   std::string_view declared_domain = "(-inf,+inf)") {
  out.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_double_entry(
          std::string(key), value, std::string(declared_domain)));
}

static void append_lls_bool_(runtime_lls_document_t &out, std::string_view key,
                             bool value) {
  out.entries.push_back(
      cuwacunu::piaabo::latent_lineage_state::make_runtime_lls_bool_entry(
          std::string(key), value));
}

[[nodiscard]] std::string build_run_report_lls_() const {
  const auto metrics = collect_epoch_report_metrics_();
  const auto &mx = metrics.matrix;
  runtime_lls_document_t out{};
  append_lls_str_(out, "schema", kRunSchema);
  append_lls_str_(out, "report.section", "transfer_matrix_evaluation.summary");
  append_lls_str_(out, "report.version", "2");
  append_lls_str_(out, "runtime.contract_hash", contract_hash_);
  append_lls_u64_(out, "runtime.samples.seen_count", epoch_.seen);
  append_lls_u64_(out, "runtime.samples.accepted_count", epoch_.accepted);
  append_lls_u64_(out, "runtime.samples.invalid_count", metrics.invalid);
  append_lls_u64_(out, "runtime.batches.buffered_count",
                  metrics.buffered_batches);
  append_lls_u64_(out, "runtime.train.batches_attempted_count",
                  epoch_.train_batches_attempted);
  append_lls_u64_(out, "runtime.train.optimizer_step_count",
                  epoch_.train_steps);
  append_lls_double_(out, "runtime.train.ingested_position_count",
                     epoch_.train_ingested_positions, "(0,+inf)");
  append_lls_double_(out, "runtime.train.ingested_support",
                     epoch_.train_ingested_support, "(0,+inf)");
  append_lls_u64_(out, "runtime.anomaly.null_cargo_count", epoch_.null_cargo);
  append_lls_u64_(out, "runtime.anomaly.invalid_cargo_count",
                  epoch_.invalid_cargo);
  append_lls_u64_(out, "runtime.anomaly.missing_encoding_count",
                  epoch_.missing_encoding);
  append_lls_u64_(out, "runtime.anomaly.non_finite_encoding_count",
                  epoch_.non_finite_encoding);
  append_lls_u64_(out, "runtime.anomaly.temporal_overlap_count",
                  epoch_.temporal_overlap);
  append_lls_u64_(out, "runtime.anomaly.shape_mismatch_count",
                  epoch_.shape_mismatch);
  append_lls_u64_(out, "runtime.anomaly.optimizer_failure_count",
                  epoch_.optimizer_failures);
  auto emit_row_summary = [&out](std::string_view row_prefix,
                                 const row_result_t &row) {
    const std::string prefix(row_prefix);
    if (row.has_support) {
      append_lls_double_(out, prefix + ".support", row.support, "(0,+inf)");
    }
    if (row.has_effort) {
      append_lls_double_(out, prefix + ".effort", row.effort, "(0,+inf)");
    }
    if (row.has_error) {
      append_lls_double_(out, prefix + ".error", row.error, "(0,+inf)");
    }
    if (row.has_effort_skill) {
      append_lls_double_(out, prefix + ".effort_skill", row.effort_skill,
                         "(-inf,+inf)");
    }
    if (row.has_error_skill) {
      append_lls_double_(out, prefix + ".error_skill", row.error_skill,
                         "(-inf,+inf)");
    }
    if (row.has_n90) {
      append_lls_double_(out, prefix + ".n90", row.n90, "(0,+inf)");
    }
    if (row.has_selectivity) {
      append_lls_double_(out, prefix + ".selectivity", row.selectivity,
                         "(-inf,+inf)");
    }
  };
  emit_row_summary("matrix.forecast.null", mx.forecast_null);
  emit_row_summary("matrix.forecast.stats_only", mx.forecast_stats_only);
  emit_row_summary("matrix.forecast.raw_linear", mx.forecast_raw_linear);
  emit_row_summary("matrix.forecast.linear", mx.forecast_linear);
  emit_row_summary("matrix.forecast.residual_linear",
                   mx.forecast_residual_linear);
  emit_row_summary("matrix.forecast.raw_mdn", mx.forecast_raw_mdn);
  emit_row_summary("matrix.forecast.mdn", mx.forecast_mdn);
  if (mx.has_cold_start_loss) {
    append_lls_double_(out, "matrix.training.cold_start_loss",
                       mx.cold_start_loss, "(0,+inf)");
  }
  if (mx.has_train_fit_loss) {
    append_lls_double_(out, "matrix.training.train_fit_loss", mx.train_fit_loss,
                       "(0,+inf)");
  }
  if (mx.has_generalization_gap) {
    append_lls_double_(out, "matrix.training.generalization_gap",
                       mx.generalization_gap, "(-inf,+inf)");
  }
  append_lls_u64_(out, "prequential.row_count.total",
                  epoch_prequential_rows_total_);
  append_lls_u64_(out, "prequential.row_count.persisted",
                  static_cast<std::uint64_t>(epoch_prequential_rows_.size()));
  append_lls_bool_(out, "prequential.row_count.truncated",
                   epoch_prequential_rows_truncated_);
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      out);
}

[[nodiscard]] std::string build_run_matrix_report_lls_() const {
  const auto &mx = epoch_matrix_;
  runtime_lls_document_t out{};
  append_lls_str_(out, "schema", kMatrixSchema);
  append_lls_str_(out, "report.section", "transfer_matrix_evaluation.matrix");
  append_lls_str_(out, "report.version", "2");
  append_lls_str_(out, "runtime.contract_hash", contract_hash_);
  auto emit_row = [&out](std::string_view prefix, const row_result_t &row) {
    if (row.has_support) {
      append_lls_double_(out, std::string(prefix).append(".support"),
                         row.support, "(0,+inf)");
    }
    if (row.has_effort) {
      append_lls_double_(out, std::string(prefix).append(".effort"), row.effort,
                         "(0,+inf)");
    }
    if (row.has_error) {
      append_lls_double_(out, std::string(prefix).append(".error"), row.error,
                         "(0,+inf)");
    }
    if (row.has_effort_skill) {
      append_lls_double_(out, std::string(prefix).append(".effort_skill"),
                         row.effort_skill, "(-inf,+inf)");
    }
    if (row.has_error_skill) {
      append_lls_double_(out, std::string(prefix).append(".error_skill"),
                         row.error_skill, "(-inf,+inf)");
    }
    if (row.has_n90) {
      append_lls_double_(out, std::string(prefix).append(".n90"), row.n90,
                         "(0,+inf)");
    }
    if (row.has_selectivity) {
      append_lls_double_(out, std::string(prefix).append(".selectivity"),
                         row.selectivity, "(-inf,+inf)");
    }
  };
  emit_row("matrix.forecast.null", mx.forecast_null);
  emit_row("matrix.forecast.stats_only", mx.forecast_stats_only);
  emit_row("matrix.forecast.raw_linear", mx.forecast_raw_linear);
  emit_row("matrix.forecast.linear", mx.forecast_linear);
  emit_row("matrix.forecast.residual_linear", mx.forecast_residual_linear);
  emit_row("matrix.forecast.raw_mdn", mx.forecast_raw_mdn);
  emit_row("matrix.forecast.mdn", mx.forecast_mdn);
  if (mx.has_cold_start_loss) {
    append_lls_double_(out, "matrix.training.cold_start_loss",
                       mx.cold_start_loss, "(0,+inf)");
  }
  if (mx.has_train_fit_loss) {
    append_lls_double_(out, "matrix.training.train_fit_loss", mx.train_fit_loss,
                       "(0,+inf)");
  }
  if (mx.has_generalization_gap) {
    append_lls_double_(out, "matrix.training.generalization_gap",
                       mx.generalization_gap, "(-inf,+inf)");
  }
  append_lls_u64_(out, "prequential.row_count.total",
                  epoch_prequential_rows_total_);
  append_lls_u64_(out, "prequential.row_count.persisted",
                  static_cast<std::uint64_t>(epoch_prequential_rows_.size()));
  append_lls_bool_(out, "prequential.row_count.truncated",
                   epoch_prequential_rows_truncated_);
  for (std::size_t i = 0; i < epoch_prequential_rows_.size(); ++i) {
    const auto &row = epoch_prequential_rows_[i];
    std::ostringstream pfx;
    pfx << "prequential." << std::setw(4) << std::setfill('0') << i;
    const std::string k = pfx.str();
    append_lls_str_(out, k + ".method", row.method);
    append_lls_u64_(out, k + ".block_index", row.block_index);
    append_lls_double_(out, k + ".prefix_support", row.prefix_support,
                       "(0,+inf)");
    append_lls_double_(out, k + ".eval_support", row.eval_support, "(0,+inf)");
    append_lls_double_(out, k + ".model_bits", row.model_bits, "(0,+inf)");
    append_lls_double_(out, k + ".null_bits", row.null_bits, "(0,+inf)");
    append_lls_double_(out, k + ".skill_bits", row.skill_bits, "(-inf,+inf)");
  }
  activation_report_t activation{};
  if (summarize_epoch_activation_report_(&activation) && activation.valid) {
    append_lls_u64_(out, "activation.sample_count", activation.sample_count);
    append_lls_u64_(out, "activation.encoding_dims", activation.encoding_dims);
    append_lls_double_(out, "activation.finite_ratio", activation.finite_ratio,
                       "(0,1)");
    append_lls_double_(out, "activation.mean", activation.mean, "(-inf,+inf)");
    append_lls_double_(out, "activation.stddev", activation.stddev, "(0,+inf)");
    append_lls_double_(out, "activation.l2_rms", activation.l2_rms, "(0,+inf)");
    append_lls_double_(out, "activation.abs_p50", activation.abs_p50,
                       "(0,+inf)");
    append_lls_double_(out, "activation.abs_p90", activation.abs_p90,
                       "(0,+inf)");
    append_lls_double_(out, "activation.abs_p99", activation.abs_p99,
                       "(0,+inf)");
    append_lls_double_(out, "activation.dead_dim_ratio",
                       activation.dead_dim_ratio, "(0,1)");
    append_lls_double_(out, "activation.cov_effective_rank",
                       activation.cov_effective_rank, "(0,+inf)");
  }
  return cuwacunu::piaabo::latent_lineage_state::emit_runtime_lls_canonical(
      out);
}

[[nodiscard]] std::string build_run_matrix_debug_string_(bool use_color) const {
  const auto &mx = epoch_matrix_;
  const char *title_color = use_color ? ANSI_COLOR_Bright_Cyan : "";
  const char *header_color = use_color ? ANSI_COLOR_Bright_Blue : "";
  const char *label_color = use_color ? ANSI_COLOR_Bright_White : "";
  const char *reset = use_color ? ANSI_COLOR_RESET : "";
  const char *dim = use_color ? ANSI_COLOR_Dim_White : "";
  const char *pos = use_color ? ANSI_COLOR_Bright_Green : "";
  const char *neg = use_color ? ANSI_COLOR_Bright_Red : "";

  struct debug_cell_t {
    std::string text{};
    const char *color{""};
    bool left_align{false};
  };

  auto fmt_value = [](bool has, double value) {
    if (!has || !std::isfinite(value))
      return std::string("n/a");
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(6);
    oss << value;
    return oss.str();
  };

  auto fmt_u64 = [](std::uint64_t value) { return std::to_string(value); };

  auto skill_color = [&](bool has, double value) -> const char * {
    if (!use_color || !has || !std::isfinite(value))
      return dim;
    return value >= 0.0 ? pos : neg;
  };

  auto colorize = [&](std::string text, const char *color) {
    if (!use_color || color == nullptr || *color == '\0')
      return text;
    return std::string(color).append(text).append(reset);
  };

  auto pad_cell = [](std::string text, std::size_t width, bool left_align) {
    if (text.size() >= width)
      return text;
    const std::size_t pad = width - text.size();
    if (left_align) {
      text.append(pad, ' ');
    } else {
      text.insert(0, pad, ' ');
    }
    return text;
  };

  auto measure_widths = [](const std::vector<debug_cell_t> &header,
                           const std::vector<std::vector<debug_cell_t>> &rows) {
    std::vector<std::size_t> widths(header.size(), 0);
    for (std::size_t i = 0; i < header.size(); ++i) {
      widths[i] = header[i].text.size();
    }
    for (const auto &row : rows) {
      for (std::size_t i = 0; i < row.size(); ++i) {
        widths[i] = std::max(widths[i], row[i].text.size());
      }
    }
    return widths;
  };

  std::ostringstream out;
  auto emit_rule = [&](const std::vector<std::size_t> &widths) {
    std::string rule = "+";
    for (const std::size_t width : widths) {
      rule.append(width + 2, '-');
      rule.push_back('+');
    }
    out << colorize(std::move(rule), header_color) << "\n";
  };

  auto emit_row = [&](const std::vector<debug_cell_t> &row,
                      const std::vector<std::size_t> &widths,
                      const char *default_color) {
    out << "|";
    for (std::size_t i = 0; i < row.size(); ++i) {
      const char *cell_color =
          (row[i].color != nullptr && *row[i].color != '\0') ? row[i].color
                                                             : default_color;
      out << " "
          << colorize(pad_cell(row[i].text, widths[i], row[i].left_align),
                      cell_color)
          << " |";
    }
    out << "\n";
  };

  auto value_cell = [&](bool has, double value) {
    return debug_cell_t{
        .text = fmt_value(has, value),
        .color = (!has || !std::isfinite(value)) ? dim : "",
        .left_align = false,
    };
  };

  auto skill_cell = [&](bool has, double value) {
    return debug_cell_t{
        .text = fmt_value(has, value),
        .color = skill_color(has, value),
        .left_align = false,
    };
  };

  out << title_color << "transfer_matrix_eval.matrix" << reset
      << " schema=" << kMatrixSchema << " contract_hash=" << contract_hash_
      << "\n";

  const std::vector<debug_cell_t> matrix_header{
      {"method", "", true},        {"support", "", false},
      {"effort", "", false},       {"error", "", false},
      {"effort_skill", "", false}, {"error_skill", "", false},
      {"n90", "", false},          {"selectivity", "", false},
  };
  std::vector<std::vector<debug_cell_t>> matrix_rows{};
  auto append_matrix_row = [&](std::string_view method,
                               const row_result_t &row) {
    matrix_rows.push_back({
        debug_cell_t{std::string(method), label_color, true},
        value_cell(row.has_support, row.support),
        value_cell(row.has_effort, row.effort),
        value_cell(row.has_error, row.error),
        skill_cell(row.has_effort_skill, row.effort_skill),
        skill_cell(row.has_error_skill, row.error_skill),
        value_cell(row.has_n90, row.n90),
        value_cell(row.has_selectivity, row.selectivity),
    });
  };

  append_matrix_row("forecast.null", mx.forecast_null);
  append_matrix_row("forecast.stats_only", mx.forecast_stats_only);
  append_matrix_row("forecast.raw_linear", mx.forecast_raw_linear);
  append_matrix_row("forecast.linear", mx.forecast_linear);
  append_matrix_row("forecast.residual_linear", mx.forecast_residual_linear);
  append_matrix_row("forecast.raw_mdn", mx.forecast_raw_mdn);
  append_matrix_row("forecast.mdn", mx.forecast_mdn);

  const auto matrix_widths = measure_widths(matrix_header, matrix_rows);
  emit_rule(matrix_widths);
  emit_row(matrix_header, matrix_widths, header_color);
  emit_rule(matrix_widths);
  for (const auto &row : matrix_rows) {
    emit_row(row, matrix_widths, "");
  }
  emit_rule(matrix_widths);

  out << header_color << "training_losses" << reset << "\n";
  const std::vector<debug_cell_t> training_header{
      {"cold_start", "", false},
      {"train_fit", "", false},
      {"generalization_gap", "", false},
  };
  const std::vector<std::vector<debug_cell_t>> training_rows{{
      value_cell(mx.has_cold_start_loss, mx.cold_start_loss),
      value_cell(mx.has_train_fit_loss, mx.train_fit_loss),
      value_cell(mx.has_generalization_gap, mx.generalization_gap),
  }};
  const auto training_widths = measure_widths(training_header, training_rows);
  emit_rule(training_widths);
  emit_row(training_header, training_widths, header_color);
  emit_rule(training_widths);
  emit_row(training_rows.front(), training_widths, "");
  emit_rule(training_widths);

  out << header_color << "prequential_rows" << reset << "\n";
  const std::vector<debug_cell_t> prequential_meta_header{
      {"total", "", false},
      {"persisted", "", false},
      {"truncated", "", false},
  };
  const std::vector<std::vector<debug_cell_t>> prequential_meta_rows{{
      debug_cell_t{fmt_u64(epoch_prequential_rows_total_), "", false},
      debug_cell_t{fmt_u64(epoch_prequential_rows_.size()), "", false},
      debug_cell_t{epoch_prequential_rows_truncated_ ? "true" : "false",
                   epoch_prequential_rows_truncated_ ? neg : pos, false},
  }};
  const auto prequential_meta_widths =
      measure_widths(prequential_meta_header, prequential_meta_rows);
  emit_rule(prequential_meta_widths);
  emit_row(prequential_meta_header, prequential_meta_widths, header_color);
  emit_rule(prequential_meta_widths);
  emit_row(prequential_meta_rows.front(), prequential_meta_widths, "");
  emit_rule(prequential_meta_widths);

  const std::vector<debug_cell_t> prequential_header{
      {"idx", "", true},           {"method", "", true},
      {"block", "", false},        {"prefix_support", "", false},
      {"eval_support", "", false}, {"model_bits", "", false},
      {"null_bits", "", false},    {"skill_bits", "", false},
  };
  std::vector<std::vector<debug_cell_t>> prequential_rows{};
  prequential_rows.reserve(epoch_prequential_rows_.size());
  for (std::size_t i = 0; i < epoch_prequential_rows_.size(); ++i) {
    const auto &row = epoch_prequential_rows_[i];
    std::ostringstream idx;
    idx << "p" << std::setw(4) << std::setfill('0') << i;
    prequential_rows.push_back({
        debug_cell_t{idx.str(), dim, true},
        debug_cell_t{row.method, label_color, true},
        debug_cell_t{fmt_u64(row.block_index), "", false},
        value_cell(/*has=*/true, row.prefix_support),
        value_cell(/*has=*/true, row.eval_support),
        value_cell(/*has=*/true, row.model_bits),
        value_cell(/*has=*/true, row.null_bits),
        skill_cell(/*has=*/true, row.skill_bits),
    });
  }
  const auto prequential_widths =
      measure_widths(prequential_header, prequential_rows);
  emit_rule(prequential_widths);
  emit_row(prequential_header, prequential_widths, header_color);
  emit_rule(prequential_widths);
  if (prequential_rows.empty()) {
    std::vector<debug_cell_t> empty_row = prequential_header;
    for (std::size_t i = 0; i < empty_row.size(); ++i) {
      empty_row[i].text = (i == 0) ? "(none)" : "";
      empty_row[i].color = dim;
      empty_row[i].left_align = true;
    }
    emit_row(empty_row, prequential_widths, "");
  } else {
    for (const auto &row : prequential_rows) {
      emit_row(row, prequential_widths, "");
    }
  }
  emit_rule(prequential_widths);
  return out.str();
}

[[nodiscard]] std::string build_run_report_string_(bool beautify) const {
  if (!beautify)
    return build_run_report_lls_();
  const auto &mx = epoch_matrix_;
  std::ostringstream out;
  out << "\033[96mtransfer_matrix_eval.run\033[0m\n";
  out << "\tcontract_hash: " << contract_hash_ << "\n";
  out << "\tschema: " << kRunSchema << "\n";
  out << "\tseen=" << epoch_.seen << " accepted=" << epoch_.accepted
      << " train_steps=" << epoch_.train_steps
      << " optimizer_failures=" << epoch_.optimizer_failures << "\n";
  out << "\tforecast.null.error="
      << (mx.forecast_null.has_error ? std::to_string(mx.forecast_null.error)
                                     : std::string("n/a"))
      << " forecast.raw_linear.error="
      << (mx.forecast_raw_linear.has_error
              ? std::to_string(mx.forecast_raw_linear.error)
              : std::string("n/a"))
      << " forecast.linear.error="
      << (mx.forecast_linear.has_error
              ? std::to_string(mx.forecast_linear.error)
              : std::string("n/a"))
      << " forecast.raw_mdn.error="
      << (mx.forecast_raw_mdn.has_error
              ? std::to_string(mx.forecast_raw_mdn.error)
              : std::string("n/a"))
      << " forecast.mdn.error="
      << (mx.forecast_mdn.has_error ? std::to_string(mx.forecast_mdn.error)
                                    : std::string("n/a"))
      << "\n";
  return out.str();
}

void emit_meta_(const Wave &wave, Emitter &out, std::string msg) const {
  out.emit_string(wave, ::tsiemene::directive_id::Meta, std::move(msg));
}

void validate_evaluation_policy_or_throw_() const {
  if (policy_.training_window !=
      cuwacunu::camahjucunu::iitepi_wave_evaluation_training_window_e::
          IncomingBatch) {
    throw std::runtime_error(
        "transfer_matrix_eval unsupported evaluation policy TRAINING_WINDOW");
  }
  if (policy_.report_policy !=
      cuwacunu::camahjucunu::iitepi_wave_evaluation_report_policy_e::
          EpochEndLog) {
    throw std::runtime_error(
        "transfer_matrix_eval unsupported evaluation policy REPORT_POLICY");
  }
  if (policy_.objective !=
      cuwacunu::camahjucunu::iitepi_wave_evaluation_objective_e::
          FutureTargetFeatureIndicesNll) {
    throw std::runtime_error(
        "transfer_matrix_eval unsupported evaluation policy OBJECTIVE");
  }
}

void initialize_runtime_model_or_throw_() {
  const auto contract =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash_);
  const auto infer_target_feature_indices_from_observation =
      [](const cuwacunu::camahjucunu::observation_spec_t &observation)
      -> std::vector<std::int64_t> {
    const auto normalize_ascii = [](std::string_view text) {
      std::string out;
      out.reserve(text.size());
      for (const unsigned char ch : text) {
        if (std::isspace(ch))
          continue;
        out.push_back(static_cast<char>(std::tolower(ch)));
      }
      return out;
    };
    const auto parse_bool_ascii = [&](std::string_view text, bool *out) {
      if (!out)
        return false;
      const std::string value = normalize_ascii(text);
      if (value == "1" || value == "true" || value == "yes" || value == "on") {
        *out = true;
        return true;
      }
      if (value == "0" || value == "false" || value == "no" || value == "off") {
        *out = false;
        return true;
      }
      return false;
    };

    std::string active_record_type{};
    for (const auto &ch : observation.channel_forms) {
      bool active = false;
      if (!parse_bool_ascii(ch.active, &active) || !active)
        continue;
      const std::string record_type = normalize_ascii(ch.record_type);
      if (record_type.empty())
        return {};
      if (active_record_type.empty()) {
        active_record_type = record_type;
      } else if (active_record_type != record_type) {
        return {};
      }
    }
    if (active_record_type.empty())
      return {};

    const auto &feature_names =
        cuwacunu::piaabo::torch_compat::data_feature_names_for_record_type(
            active_record_type);
    std::vector<std::int64_t> indices{};
    indices.reserve(feature_names.size());
    for (std::size_t i = 0; i < feature_names.size(); ++i) {
      indices.push_back(static_cast<std::int64_t>(i));
    }
    return indices;
  };

  model_.encoding_dims = contract->get<int>("VICReg", "encoding_dims");
  model_.mixture_comps = cfg_.mdn_mixture_comps;
  model_.features_hidden = cfg_.mdn_features_hidden;
  model_.residual_depth = cfg_.mdn_residual_depth;
  model_.dtype = resolve_config_dtype_with_fallback_();
  model_.device = resolve_config_device_with_fallback_();

  auto observation =
      cuwacunu::camahjucunu::decode_observation_spec_from_contract(
          contract_hash_);
  model_.channels = observation.count_channels();
  model_.horizons = observation.max_future_sequence_length();
  model_.target_feature_indices =
      cfg_.mdn_target_feature_indices.empty()
          ? infer_target_feature_indices_from_observation(observation)
          : cfg_.mdn_target_feature_indices;

  const auto configured_future_feature_width = static_cast<std::int64_t>(
      contract->get<int>("DOCK", "__obs_feature_dim", std::optional<int>{0}));
  {
    auto sorted_indices = model_.target_feature_indices;
    std::sort(sorted_indices.begin(), sorted_indices.end());
    if (sorted_indices.empty()) {
      throw std::runtime_error(
          "transfer_matrix_eval target_feature_indices is empty");
    }
    if (sorted_indices.front() < 0) {
      throw std::runtime_error(
          "transfer_matrix_eval target_feature_indices must be non-negative");
    }
    if (std::adjacent_find(sorted_indices.begin(), sorted_indices.end()) !=
        sorted_indices.end()) {
      throw std::runtime_error(
          "transfer_matrix_eval target_feature_indices must be unique");
    }
    if (configured_future_feature_width > 0 &&
        sorted_indices.back() >= configured_future_feature_width) {
      throw std::runtime_error(
          "transfer_matrix_eval target_feature_indices must fit future feature "
          "width");
    }
  }

  if (model_.encoding_dims <= 0 || model_.mixture_comps <= 0 ||
      model_.features_hidden <= 0 || model_.residual_depth < 0 ||
      model_.channels <= 0 || model_.horizons <= 0 ||
      model_.target_feature_indices.empty()) {
    throw std::runtime_error(
        "transfer_matrix_eval invalid model specification from contract");
  }

  const auto channel_weights = observation.retrieve_channel_weights();
  if (!channel_weights.empty() &&
      static_cast<std::int64_t>(channel_weights.size()) == model_.channels) {
    channel_weights_ = torch::tensor(
        channel_weights,
        torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  }

  initialize_runtime_model_for_input_dims_or_throw_(
      model_.encoding_dims, &semantic_model_, &optimizer_);
}

void initialize_runtime_model_for_input_dims_or_throw_(
    std::int64_t input_dims, cuwacunu::wikimyei::mdn::MdnModel *out_model,
    std::unique_ptr<torch::optim::Optimizer> *out_optimizer) const {
  if (!out_model || !out_optimizer) {
    throw std::runtime_error(
        "transfer_matrix_eval output model storage is missing");
  }
  if (input_dims <= 0 || model_.mixture_comps <= 0 ||
      model_.features_hidden <= 0 || model_.residual_depth < 0 ||
      model_.channels <= 0 || model_.horizons <= 0 ||
      model_.target_feature_indices.empty()) {
    throw std::runtime_error(
        "transfer_matrix_eval invalid model specification from contract");
  }

  *out_model = cuwacunu::wikimyei::mdn::MdnModel(
      input_dims,
      static_cast<std::int64_t>(model_.target_feature_indices.size()),
      model_.channels, model_.horizons, model_.mixture_comps,
      model_.features_hidden, model_.residual_depth, model_.dtype,
      model_.device,
      /*display_model=*/false);

  std::vector<torch::Tensor> params;
  params.reserve(64);
  for (auto &p : (*out_model)->parameters(/*recurse=*/true)) {
    if (p.requires_grad())
      params.push_back(p);
  }

  if (params.empty()) {
    throw std::runtime_error(
        "transfer_matrix_eval model has no trainable parameters");
  }

  auto opts =
      torch::optim::AdamWOptions(cfg_.optimizer_lr)
          .weight_decay(cfg_.optimizer_weight_decay)
          .betas(std::make_tuple(cfg_.optimizer_beta1, cfg_.optimizer_beta2))
          .eps(cfg_.optimizer_eps);
  *out_optimizer = std::make_unique<torch::optim::AdamW>(params, opts);
}

[[nodiscard]] torch::Dtype resolve_config_dtype_with_fallback_() const {
  try {
    return cuwacunu::iitepi::config_dtype(contract_hash_, kContractSection);
  } catch (const std::exception &) {
    try {
      return cuwacunu::iitepi::config_dtype(contract_hash_, "VICReg");
    } catch (const std::exception &) {
      try {
        return cuwacunu::iitepi::config_dtype(contract_hash_, "GENERAL");
      } catch (const std::exception &) {
        return torch::kFloat32;
      }
    }
  }
}

[[nodiscard]] torch::Device resolve_config_device_with_fallback_() const {
  try {
    return cuwacunu::iitepi::config_device(contract_hash_, kContractSection);
  } catch (const std::exception &) {
    try {
      return cuwacunu::iitepi::config_device(contract_hash_, "VICReg");
    } catch (const std::exception &) {
      try {
        return cuwacunu::iitepi::config_device(contract_hash_, "GENERAL");
      } catch (const std::exception &) {
        return torch::Device(torch::kCPU);
      }
    }
  }
}

void load_config_() {
  const auto contract =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash_);

  cfg_.check_temporal_order = contract->get<bool>(
      kContractSection, "check_temporal_order", std::optional<bool>{true});
  cfg_.validate_vicreg_out = contract->get<bool>(
      kContractSection, "validate_vicreg_out", std::optional<bool>{true});
  cfg_.report_shapes = contract->get<bool>(kContractSection, "report_shapes",
                                           std::optional<bool>{false});
  cfg_.mdn_mixture_comps = static_cast<std::int64_t>(contract->get<int>(
      kContractSection, "mdn_mixture_comps", std::optional<int>{1}));
  cfg_.mdn_features_hidden = static_cast<std::int64_t>(contract->get<int>(
      kContractSection, "mdn_features_hidden", std::optional<int>{16}));
  cfg_.mdn_residual_depth = static_cast<std::int64_t>(contract->get<int>(
      kContractSection, "mdn_residual_depth", std::optional<int>{0}));
  cfg_.mdn_target_feature_indices = contract->get_arr<std::int64_t>(
      kContractSection, "mdn_target_feature_indices",
      std::optional<std::vector<std::int64_t>>{std::vector<std::int64_t>{}});

  cfg_.optimizer_lr = contract->get<double>(kContractSection, "optimizer_lr",
                                            std::optional<double>{1e-3});
  cfg_.optimizer_weight_decay = contract->get<double>(
      kContractSection, "optimizer_weight_decay", std::optional<double>{1e-2});
  cfg_.optimizer_beta1 = contract->get<double>(
      kContractSection, "optimizer_beta1", std::optional<double>{0.9});
  cfg_.optimizer_beta2 = contract->get<double>(
      kContractSection, "optimizer_beta2", std::optional<double>{0.999});
  cfg_.optimizer_eps = contract->get<double>(kContractSection, "optimizer_eps",
                                             std::optional<double>{1e-8});
  cfg_.grad_clip = contract->get<double>(kContractSection, "grad_clip",
                                         std::optional<double>{1.0});

  cfg_.nll_eps = contract->get<double>(kContractSection, "nll_eps",
                                       std::optional<double>{1e-6});
  cfg_.nll_sigma_min = contract->get<double>(kContractSection, "nll_sigma_min",
                                             std::optional<double>{1e-3});
  cfg_.nll_sigma_max = contract->get<double>(kContractSection, "nll_sigma_max",
                                             std::optional<double>{0.0});

  cfg_.summary_every_steps = static_cast<std::int64_t>(contract->get<int>(
      kContractSection, "summary_every_steps", std::optional<int>{256}));
  cfg_.anchor_train_ratio = contract->get<double>(
      kContractSection, "anchor_train_ratio", std::optional<double>{0.70});
  cfg_.anchor_val_ratio = contract->get<double>(
      kContractSection, "anchor_val_ratio", std::optional<double>{0.15});
  cfg_.anchor_test_ratio = contract->get<double>(
      kContractSection, "anchor_test_ratio", std::optional<double>{0.15});
  cfg_.prequential_blocks = static_cast<std::int64_t>(contract->get<int>(
      kContractSection, "prequential_blocks", std::optional<int>{4}));
  cfg_.linear_ridge_lambda = contract->get<double>(
      kContractSection, "linear_ridge_lambda", std::optional<double>{1e-3});
  cfg_.gaussian_var_min = contract->get<double>(
      kContractSection, "gaussian_var_min", std::optional<double>{1e-6});
  cfg_.control_shuffle_block = static_cast<std::int64_t>(contract->get<int>(
      kContractSection, "control_shuffle_block", std::optional<int>{16}));
  cfg_.control_shuffle_seed = static_cast<std::int64_t>(contract->get<int>(
      kContractSection, "control_shuffle_seed", std::optional<int>{1337}));

  if (cfg_.mdn_mixture_comps <= 0)
    cfg_.mdn_mixture_comps = 1;
  if (cfg_.mdn_features_hidden <= 0)
    cfg_.mdn_features_hidden = 16;
  if (cfg_.mdn_residual_depth < 0)
    cfg_.mdn_residual_depth = 0;

  if (cfg_.summary_every_steps < 0)
    cfg_.summary_every_steps = 0;
  if (cfg_.prequential_blocks < 2)
    cfg_.prequential_blocks = 2;
  if (cfg_.control_shuffle_block < 1)
    cfg_.control_shuffle_block = 1;
  if (cfg_.control_shuffle_seed < 0)
    cfg_.control_shuffle_seed = 0;
  if (cfg_.optimizer_lr <= 0.0)
    cfg_.optimizer_lr = 1e-3;
  if (cfg_.optimizer_eps <= 0.0)
    cfg_.optimizer_eps = 1e-8;
  if (cfg_.nll_eps <= 0.0)
    cfg_.nll_eps = 1e-6;
  if (cfg_.nll_sigma_min < 0.0)
    cfg_.nll_sigma_min = 0.0;
  if (cfg_.nll_sigma_max < 0.0)
    cfg_.nll_sigma_max = 0.0;
  if (cfg_.gaussian_var_min <= 0.0)
    cfg_.gaussian_var_min = 1e-6;
  if (cfg_.linear_ridge_lambda < 0.0)
    cfg_.linear_ridge_lambda = 1e-3;
  cfg_.anchor_train_ratio = std::clamp(cfg_.anchor_train_ratio, 0.0, 1.0);
  cfg_.anchor_val_ratio = std::clamp(cfg_.anchor_val_ratio, 0.0, 1.0);
  cfg_.anchor_test_ratio = std::clamp(cfg_.anchor_test_ratio, 0.0, 1.0);
  const double ratio_sum =
      cfg_.anchor_train_ratio + cfg_.anchor_val_ratio + cfg_.anchor_test_ratio;
  if (ratio_sum <= 0.0) {
    cfg_.anchor_train_ratio = 0.70;
    cfg_.anchor_val_ratio = 0.15;
    cfg_.anchor_test_ratio = 0.15;
  } else {
    cfg_.anchor_train_ratio /= ratio_sum;
    cfg_.anchor_val_ratio /= ratio_sum;
    cfg_.anchor_test_ratio /= ratio_sum;
  }
  if (cfg_.optimizer_beta1 < 0.0 || cfg_.optimizer_beta1 >= 1.0) {
    cfg_.optimizer_beta1 = 0.9;
  }
  if (cfg_.optimizer_beta2 < 0.0 || cfg_.optimizer_beta2 >= 1.0) {
    cfg_.optimizer_beta2 = 0.999;
  }
}

void reset_epoch_accumulators_() {
  epoch_ = epoch_stats_t{};
  epoch_matrix_ = matrix_results_t{};
  epoch_batches_.clear();
  epoch_prequential_rows_.clear();
  epoch_prequential_rows_total_ = 0;
  epoch_prequential_rows_truncated_ = false;
}

static constexpr const char *kContractSection =
    "WIKIMYEI_EVALUATION_TRANSFER_MATRIX_EVALUATION";
static constexpr const char *kRunSchema =
    "tsi.wikimyei.representation.encoding.vicreg.transfer_matrix_evaluation."
    "run.v1";
static constexpr const char *kMatrixSchema =
    "tsi.wikimyei.representation.encoding.vicreg.transfer_matrix_evaluation."
    "matrix.v1";

std::string contract_hash_{};
EvaluationPolicy policy_{};
static constexpr std::size_t kMaxPersistedPrequentialRows_{512};
std::unordered_map<std::string, anchor_split_e> anchor_split_manifest_{};
config_t cfg_{};
model_spec_t model_{};
epoch_stats_t epoch_{};
matrix_results_t epoch_matrix_{};
std::vector<train_batch_t> epoch_batches_{};
std::vector<prequential_row_t> epoch_prequential_rows_{};
std::uint64_t epoch_prequential_rows_total_{0};
bool epoch_prequential_rows_truncated_{false};

cuwacunu::wikimyei::mdn::MdnModel semantic_model_{nullptr};
std::unique_ptr<torch::optim::Optimizer> optimizer_{};
torch::Tensor target_index_{};
torch::Tensor channel_weights_{};
std::string last_run_report_lls_{};
std::string last_run_matrix_report_lls_{};
}
;

} // namespace cuwacunu::wikimyei::evaluation
