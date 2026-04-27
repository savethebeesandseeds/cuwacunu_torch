[[nodiscard]] static std::vector<std::vector<const train_batch_t *>>
make_prequential_blocks_(const std::vector<const train_batch_t *> &batches,
                         std::int64_t requested_blocks) {
  std::vector<std::vector<const train_batch_t *>> out;
  if (batches.empty())
    return out;
  const std::size_t n = batches.size();
  const std::size_t S = static_cast<std::size_t>(
      std::max<std::int64_t>(1, std::min<std::int64_t>(requested_blocks, n)));
  out.reserve(S);
  for (std::size_t s = 0; s < S; ++s) {
    const std::size_t start = (s * n) / S;
    const std::size_t end = ((s + 1) * n) / S;
    if (end <= start)
      continue;
    std::vector<const train_batch_t *> block;
    block.reserve(end - start);
    for (std::size_t i = start; i < end; ++i)
      block.push_back(batches[i]);
    if (!block.empty())
      out.push_back(std::move(block));
  }
  return out;
}

[[nodiscard]] bool
evaluate_mdn_batches_(const std::vector<const train_batch_t *> &batches,
                      const target_norm_t *target_norm, double *out_bits,
                      double *out_weight, std::string *error) {
  return evaluate_mdn_batches_for_model_(batches, feature_mode_e::Encoding,
                                         target_norm, semantic_model_, out_bits,
                                         out_weight, error);
}

[[nodiscard]] bool evaluate_mdn_batches_for_model_(
    const std::vector<const train_batch_t *> &batches, feature_mode_e mode,
    const target_norm_t *target_norm, cuwacunu::wikimyei::mdn::MdnModel &model,
    double *out_bits, double *out_weight, std::string *error) {
  if (error)
    error->clear();
  if (out_bits)
    *out_bits = std::numeric_limits<double>::quiet_NaN();
  if (out_weight)
    *out_weight = 0.0;
  if (batches.empty())
    return false;

  const double dy = std::max<double>(
      1.0, static_cast<double>(model_.target_feature_indices.size()));
  double weighted_sum = 0.0;
  double weight_sum = 0.0;
  for (const auto *batch : batches) {
    if (!batch)
      continue;
    double nll_bits = std::numeric_limits<double>::quiet_NaN();
    std::string batch_error;
    if (!compute_batch_nll_for_model_(*batch, mode, target_norm, model,
                                      &nll_bits, &batch_error)) {
      if (error && error->empty())
        *error = batch_error;
      continue;
    }
    double weight = 0.0;
    try {
      weight = batch->future_mask.sum().item<double>() * dy;
    } catch (...) {
      weight = 0.0;
    }
    if (!std::isfinite(weight) || weight <= 0.0)
      continue;
    weighted_sum += nll_bits * weight;
    weight_sum += weight;
  }
  if (weight_sum <= 0.0)
    return false;
  if (out_bits)
    *out_bits = weighted_sum / weight_sum;
  if (out_weight)
    *out_weight = weight_sum;
  return true;
}

void train_mdn_batches_(const std::vector<const train_batch_t *> &batches,
                        const target_norm_t *target_norm) {
  train_mdn_batches_for_model_(batches, feature_mode_e::Encoding, target_norm,
                               semantic_model_, optimizer_.get(),
                               /*record_training_stats=*/true);
}

void train_mdn_batches_for_model_(
    const std::vector<const train_batch_t *> &batches, feature_mode_e mode,
    const target_norm_t *target_norm, cuwacunu::wikimyei::mdn::MdnModel &model,
    torch::optim::Optimizer *optimizer, bool record_training_stats) {
  for (const auto *batch : batches) {
    if (!batch)
      continue;
    if (record_training_stats)
      ++epoch_.train_batches_attempted;
    std::string train_error;
    if (!train_one_batch_for_model_(*batch, mode, target_norm, model, optimizer,
                                    record_training_stats, &train_error)) {
      if (record_training_stats)
        ++epoch_.optimizer_failures;
    }
  }
}

[[nodiscard]] bool
flatten_batches_feature_(const std::vector<const train_batch_t *> &batches,
                         feature_mode_e mode, flat_dataset_t *out) const {
  if (!out)
    return false;
  std::vector<torch::Tensor> xs{};
  std::vector<torch::Tensor> ys{};
  std::vector<torch::Tensor> ws{};
  xs.reserve(batches.size());
  ys.reserve(batches.size());
  ws.reserve(batches.size());

  for (const auto *batch : batches) {
    if (!batch || !batch->target.defined() || !batch->future_mask.defined()) {
      continue;
    }
    const auto *src = feature_tensor_ptr_(*batch, mode);
    if (!src || !src->defined() || src->numel() == 0)
      continue;
    auto enc = src->to(torch::kCPU, torch::kFloat64);
    auto tgt = batch->target.to(torch::kCPU, torch::kFloat64);
    auto m = batch->future_mask.to(torch::kCPU, torch::kFloat64);
    if (enc.dim() != 2 || tgt.dim() != 4 || m.dim() != 3)
      continue;

    const auto B = enc.size(0);
    const auto C = tgt.size(1);
    const auto Hf = tgt.size(2);
    const auto Df = tgt.size(3);
    if (B <= 0 || C <= 0 || Hf <= 0 || Df <= 0)
      continue;
    if (tgt.size(0) != B || m.size(0) != B || m.size(1) != C ||
        m.size(2) != Hf) {
      continue;
    }
    auto x = enc.unsqueeze(1)
                 .unsqueeze(1)
                 .expand({B, C, Hf, enc.size(1)})
                 .reshape({B * C * Hf, enc.size(1)});
    auto y = tgt.reshape({B * C * Hf, Df});
    auto w = m.reshape({B * C * Hf});
    xs.push_back(std::move(x));
    ys.push_back(std::move(y));
    ws.push_back(std::move(w));
  }

  if (xs.empty()) {
    out->x.reset();
    out->y.reset();
    out->w.reset();
    return false;
  }
  out->x = torch::cat(xs, 0);
  out->y = torch::cat(ys, 0);
  out->w = torch::cat(ws, 0).clamp_min(0.0);
  return out->x.defined() && out->x.numel() > 0 && out->y.defined() &&
         out->y.numel() > 0 && out->w.defined() && out->w.numel() > 0;
}

[[nodiscard]] bool
flatten_batches_(const std::vector<const train_batch_t *> &batches,
                 flat_dataset_t *out) const {
  return flatten_batches_feature_(batches, feature_mode_e::Encoding, out);
}

[[nodiscard]] bool
flatten_batches_stats_(const std::vector<const train_batch_t *> &batches,
                       flat_dataset_t *out) const {
  return flatten_batches_feature_(batches, feature_mode_e::Stats, out);
}

[[nodiscard]] bool
flatten_batches_raw_(const std::vector<const train_batch_t *> &batches,
                     flat_dataset_t *out) const {
  return flatten_batches_feature_(batches, feature_mode_e::Raw, out);
}

[[nodiscard]] static std::int64_t
feature_dims_from_batches_(const std::vector<const train_batch_t *> &batches,
                           feature_mode_e mode) {
  for (const auto *batch : batches) {
    if (!batch)
      continue;
    const auto *src = feature_tensor_ptr_(*batch, mode);
    if (!src || !src->defined() || src->dim() != 2 || src->size(1) <= 0)
      continue;
    return src->size(1);
  }
  return 0;
}

[[nodiscard]] static std::vector<const train_batch_t *>
ptr_view_(const std::vector<train_batch_t> &batches) {
  std::vector<const train_batch_t *> out{};
  out.reserve(batches.size());
  for (const auto &b : batches)
    out.push_back(&b);
  return out;
}

[[nodiscard]] bool
support_elements_(const std::vector<const train_batch_t *> &batches,
                  double *out_support) const {
  if (out_support)
    *out_support = 0.0;
  flat_dataset_t ds{};
  if (!flatten_batches_(batches, &ds))
    return false;
  const double w_sum = ds.w.sum().item<double>();
  if (!std::isfinite(w_sum) || w_sum <= 0.0)
    return false;
  const double dy = static_cast<double>(ds.y.size(1));
  const double support = w_sum * std::max(1.0, dy);
  if (!std::isfinite(support) || support <= 0.0)
    return false;
  if (out_support)
    *out_support = support;
  return true;
}

static void append_prefix_blocks_(
    const std::vector<std::vector<const train_batch_t *>> &blocks,
    std::size_t block_exclusive,
    std::vector<const train_batch_t *> *out_prefix) {
  if (!out_prefix)
    return;
  out_prefix->clear();
  for (std::size_t j = 0; j < block_exclusive; ++j) {
    out_prefix->insert(out_prefix->end(), blocks[j].begin(), blocks[j].end());
  }
}

[[nodiscard]] static bool
fit_linear_gaussian_from_dataset_(const flat_dataset_t &ds, double ridge_lambda,
                                  double var_floor, linear_gaussian_t *out,
                                  std::string *error) {
  if (error)
    error->clear();
  if (!out)
    return false;
  if (!ds.x.defined() || !ds.y.defined() || !ds.w.defined()) {
    if (error)
      *error = "undefined dataset tensors";
    return false;
  }
  if (ds.x.dim() != 2 || ds.y.dim() != 2 || ds.w.dim() != 1) {
    if (error)
      *error = "invalid dataset rank";
    return false;
  }
  if (ds.x.size(0) != ds.y.size(0) || ds.x.size(0) != ds.w.size(0)) {
    if (error)
      *error = "dataset shape mismatch";
    return false;
  }

  const auto N = ds.x.size(0);
  const auto De = ds.x.size(1);
  const auto Df = ds.y.size(1);
  if (N <= 0 || De <= 0 || Df <= 0) {
    if (error)
      *error = "empty dataset dimensions";
    return false;
  }

  try {
    auto opts =
        torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU);
    auto ones = torch::ones(std::vector<std::int64_t>{N, 1}, opts);
    auto x_aug = torch::cat(std::vector<torch::Tensor>{ds.x, ones}, /*dim=*/1);
    auto w = ds.w.clamp_min(0.0);
    const double w_sum = w.sum().item<double>();
    if (!std::isfinite(w_sum) || w_sum <= 0.0) {
      if (error)
        *error = "invalid sample weights";
      return false;
    }

    auto sqrt_w = torch::sqrt(w).unsqueeze(1);
    auto xw = x_aug * sqrt_w;
    auto yw = ds.y * sqrt_w;

    const auto P = x_aug.size(1);
    auto xtx = xw.transpose(0, 1).mm(xw);
    auto xty = xw.transpose(0, 1).mm(yw);
    auto reg = torch::eye(P, opts) * std::max(0.0, ridge_lambda);
    reg.index_put_({P - 1, P - 1}, 0.0); // keep bias unregularized

    auto lhs = xtx + reg;
    torch::Tensor beta;
    try {
      beta = at::linalg_solve(lhs, xty, /*left=*/true);
    } catch (...) {
      beta = at::linalg_pinv(lhs).mm(xty);
    }

    auto pred = x_aug.mm(beta);
    auto resid = ds.y - pred;
    auto w_col = w.unsqueeze(1);
    const double denom = std::max<double>(w_sum * static_cast<double>(Df), 1.0);
    const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;

    out->beta = beta;
    out->sigma2 = std::max(mse, var_floor);
    out->valid = out->beta.defined() && out->beta.numel() > 0 &&
                 std::isfinite(out->sigma2) && out->sigma2 > 0.0;
    return out->valid;
  } catch (const std::exception &e) {
    if (error)
      *error = e.what();
    return false;
  }
}

[[nodiscard]] static torch::Tensor
predict_linear_from_dataset_(const linear_gaussian_t &model,
                             const torch::Tensor &x) {
  TORCH_CHECK(model.valid && model.beta.defined(),
              "[transfer_matrix_eval] linear model invalid");
  TORCH_CHECK(x.defined() && x.dim() == 2, "[transfer_matrix_eval] invalid x");
  TORCH_CHECK(x.size(1) + 1 == model.beta.size(0),
              "[transfer_matrix_eval] linear model/x shape mismatch");
  auto opts = torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU);
  auto ones = torch::ones(std::vector<std::int64_t>{x.size(0), 1}, opts);
  auto x_aug = torch::cat(std::vector<torch::Tensor>{x, ones}, /*dim=*/1);
  return x_aug.mm(model.beta);
}

[[nodiscard]] static bool eval_linear_gaussian_from_dataset_(
    const linear_gaussian_t &model, const flat_dataset_t &ds, double var_floor,
    double *out_bits, double *out_weight, std::string *error) {
  if (error)
    error->clear();
  if (out_bits)
    *out_bits = std::numeric_limits<double>::quiet_NaN();
  if (out_weight)
    *out_weight = 0.0;
  if (!model.valid || !model.beta.defined()) {
    if (error)
      *error = "linear model not fitted";
    return false;
  }
  if (!ds.x.defined() || !ds.y.defined() || !ds.w.defined())
    return false;
  if (ds.x.dim() != 2 || ds.y.dim() != 2 || ds.w.dim() != 1)
    return false;
  if (ds.x.size(0) != ds.y.size(0) || ds.x.size(0) != ds.w.size(0))
    return false;
  if (ds.x.size(1) + 1 != model.beta.size(0)) {
    if (error)
      *error = "linear model/input shape mismatch";
    return false;
  }

  try {
    auto pred = predict_linear_from_dataset_(model, ds.x);
    auto resid = ds.y - pred;
    auto w = ds.w.clamp_min(0.0);
    const double w_sum = w.sum().item<double>();
    if (!std::isfinite(w_sum) || w_sum <= 0.0)
      return false;
    auto w_col = w.unsqueeze(1);
    const double denom =
        std::max<double>(w_sum * static_cast<double>(ds.y.size(1)), 1.0);
    const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;

    constexpr double kTwoPi = 6.28318530717958647692;
    constexpr double kInvLn2 = 1.4426950408889634074;
    const double sigma2 = std::max(model.sigma2, var_floor);
    const double nll_bits =
        0.5 * (std::log(kTwoPi * sigma2) + (mse / sigma2)) * kInvLn2;
    if (out_bits)
      *out_bits = nll_bits;
    if (out_weight)
      *out_weight = denom;
    return std::isfinite(nll_bits);
  } catch (const std::exception &e) {
    if (error)
      *error = e.what();
    return false;
  }
}

[[nodiscard]] static bool
apply_n90_from_curve_(const std::vector<std::pair<double, double>> &curve,
                      double final_skill, row_result_t *out) {
  if (!out)
    return false;
  if (!std::isfinite(final_skill) || final_skill <= 0.0)
    return false;
  if (curve.empty())
    return false;
  const double threshold = 0.9 * final_skill;
  for (const auto &p : curve) {
    const double n = p.first;
    const double skill = p.second;
    if (!std::isfinite(n) || !std::isfinite(skill))
      continue;
    if (skill >= threshold) {
      out->n90 = n;
      out->has_n90 = true;
      return true;
    }
  }
  return false;
}

static void
explode_batches_by_sample_(const std::vector<const train_batch_t *> &batches,
                           std::vector<train_batch_t> *out) {
  if (!out)
    return;
  out->clear();
  for (const auto *batch : batches) {
    if (!batch || !batch->encoding.defined() || !batch->target.defined() ||
        !batch->future_mask.defined() || batch->encoding.dim() != 2 ||
        batch->target.dim() != 4 || batch->future_mask.dim() != 3) {
      continue;
    }
    const auto B = batch->encoding.size(0);
    if (B <= 0 || batch->target.size(0) != B ||
        batch->future_mask.size(0) != B) {
      continue;
    }
    for (std::int64_t b = 0; b < B; ++b) {
      train_batch_t sample{};
      sample.encoding = batch->encoding.narrow(0, b, 1).clone();
      if (batch->stats.defined() && batch->stats.dim() == 2 &&
          batch->stats.size(0) == B) {
        sample.stats = batch->stats.narrow(0, b, 1).clone();
      }
      if (batch->raw.defined() && batch->raw.dim() == 2 &&
          batch->raw.size(0) == B) {
        sample.raw = batch->raw.narrow(0, b, 1).clone();
      }
      sample.target = batch->target.narrow(0, b, 1).clone();
      sample.future_mask = batch->future_mask.narrow(0, b, 1).clone();
      sample.anchor_key = batch->anchor_key + "|s=" + std::to_string(b);
      out->push_back(std::move(sample));
    }
  }
}

[[nodiscard]] static std::vector<train_batch_t>
make_block_shuffled_control_(const std::vector<const train_batch_t *> &batches,
                             std::int64_t block_size, std::uint64_t seed) {
  std::vector<train_batch_t> out{};
  out.reserve(batches.size());
  const std::int64_t bs = std::max<std::int64_t>(1, block_size);
  std::mt19937_64 rng(seed);
  for (const auto *src : batches) {
    if (!src || !src->encoding.defined() || !src->target.defined() ||
        !src->future_mask.defined() || src->target.dim() != 4 ||
        src->future_mask.dim() != 3) {
      continue;
    }
    train_batch_t cpy = *src;
    const auto B = cpy.target.size(0);
    if (B > 1 && cpy.future_mask.size(0) == B) {
      std::vector<std::int64_t> perm(static_cast<std::size_t>(B));
      std::iota(perm.begin(), perm.end(), 0);
      for (std::int64_t start = 0; start < B; start += bs) {
        const std::int64_t end = std::min<std::int64_t>(B, start + bs);
        std::shuffle(perm.begin() + start, perm.begin() + end, rng);
      }
      auto idx = torch::tensor(
          perm, torch::TensorOptions().dtype(torch::kLong).device(torch::kCPU));
      cpy.target = cpy.target.index_select(0, idx);
      cpy.future_mask = cpy.future_mask.index_select(0, idx);
    }
    cpy.anchor_key = src->anchor_key + "|ctrl";
    out.push_back(std::move(cpy));
  }
  return out;
}

[[nodiscard]] bool
fit_null_baseline_(const std::vector<const train_batch_t *> &batches,
                   gaussian_baseline_t *out, std::string *error) const {
  if (error)
    error->clear();
  if (!out)
    return false;
  flat_dataset_t ds{};
  if (!flatten_batches_(batches, &ds)) {
    if (error)
      *error = "empty dataset for null baseline";
    return false;
  }
  const double w_sum = ds.w.sum().item<double>();
  if (!std::isfinite(w_sum) || w_sum <= 0.0)
    return false;
  auto w_col = ds.w.unsqueeze(1);
  auto mu = (ds.y * w_col).sum(/*dim=*/0) / w_sum;
  auto resid = ds.y - mu;
  const double denom =
      std::max<double>(w_sum * static_cast<double>(ds.y.size(1)), 1.0);
  const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;
  out->mu = mu;
  out->sigma2 = std::max(mse, cfg_.gaussian_var_min);
  out->valid = std::isfinite(out->sigma2) && out->sigma2 > 0.0;
  return out->valid;
}

[[nodiscard]] bool
eval_null_baseline_bits_(const gaussian_baseline_t &model,
                         const std::vector<const train_batch_t *> &batches,
                         double *out_bits, double *out_weight,
                         std::string *error) const {
  if (error)
    error->clear();
  if (out_bits)
    *out_bits = std::numeric_limits<double>::quiet_NaN();
  if (out_weight)
    *out_weight = 0.0;
  if (!model.valid || !model.mu.defined())
    return false;
  flat_dataset_t ds{};
  if (!flatten_batches_(batches, &ds))
    return false;
  const double w_sum = ds.w.sum().item<double>();
  if (!std::isfinite(w_sum) || w_sum <= 0.0)
    return false;
  auto resid = ds.y - model.mu;
  auto w_col = ds.w.unsqueeze(1);
  const double denom =
      std::max<double>(w_sum * static_cast<double>(ds.y.size(1)), 1.0);
  const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;
  constexpr double kTwoPi = 6.28318530717958647692;
  constexpr double kInvLn2 = 1.4426950408889634074;
  const double sigma2 = std::max(model.sigma2, cfg_.gaussian_var_min);
  const double nll_bits =
      0.5 * (std::log(kTwoPi * sigma2) + (mse / sigma2)) * kInvLn2;
  if (out_bits)
    *out_bits = nll_bits;
  if (out_weight)
    *out_weight = denom;
  return std::isfinite(nll_bits);
}

[[nodiscard]] bool
fit_linear_gaussian_(const std::vector<const train_batch_t *> &batches,
                     linear_gaussian_t *out, std::string *error) const {
  return fit_linear_gaussian_for_mode_(batches, feature_mode_e::Encoding,
                                       "linear baseline", out, error);
}

[[nodiscard]] bool
eval_linear_gaussian_bits_(const linear_gaussian_t &model,
                           const std::vector<const train_batch_t *> &batches,
                           double *out_bits, double *out_weight,
                           std::string *error) const {
  return eval_linear_gaussian_bits_for_mode_(
      model, batches, feature_mode_e::Encoding, out_bits, out_weight, error);
}

[[nodiscard]] bool
fit_stats_linear_gaussian_(const std::vector<const train_batch_t *> &batches,
                           linear_gaussian_t *out, std::string *error) const {
  return fit_linear_gaussian_for_mode_(batches, feature_mode_e::Stats,
                                       "stats-only baseline", out, error);
}

[[nodiscard]] bool eval_stats_linear_gaussian_bits_(
    const linear_gaussian_t &model,
    const std::vector<const train_batch_t *> &batches, double *out_bits,
    double *out_weight, std::string *error) const {
  return eval_linear_gaussian_bits_for_mode_(
      model, batches, feature_mode_e::Stats, out_bits, out_weight, error);
}

[[nodiscard]] bool
fit_raw_linear_gaussian_(const std::vector<const train_batch_t *> &batches,
                         linear_gaussian_t *out, std::string *error) const {
  return fit_linear_gaussian_for_mode_(batches, feature_mode_e::Raw,
                                       "raw linear baseline", out, error);
}

[[nodiscard]] bool eval_raw_linear_gaussian_bits_(
    const linear_gaussian_t &model,
    const std::vector<const train_batch_t *> &batches, double *out_bits,
    double *out_weight, std::string *error) const {
  return eval_linear_gaussian_bits_for_mode_(
      model, batches, feature_mode_e::Raw, out_bits, out_weight, error);
}

[[nodiscard]] bool fit_linear_gaussian_for_mode_(
    const std::vector<const train_batch_t *> &batches, feature_mode_e mode,
    std::string_view label, linear_gaussian_t *out, std::string *error) const {
  if (error)
    error->clear();
  if (!out)
    return false;
  flat_dataset_t ds{};
  if (!flatten_batches_feature_(batches, mode, &ds)) {
    if (error)
      *error = "empty dataset for " + std::string(label);
    return false;
  }
  return fit_linear_gaussian_from_dataset_(ds, cfg_.linear_ridge_lambda,
                                           cfg_.gaussian_var_min, out, error);
}

[[nodiscard]] bool eval_linear_gaussian_bits_for_mode_(
    const linear_gaussian_t &model,
    const std::vector<const train_batch_t *> &batches, feature_mode_e mode,
    double *out_bits, double *out_weight, std::string *error) const {
  if (error)
    error->clear();
  if (out_bits)
    *out_bits = std::numeric_limits<double>::quiet_NaN();
  if (out_weight)
    *out_weight = 0.0;
  if (!model.valid || !model.beta.defined()) {
    if (error)
      *error = "linear model not fitted";
    return false;
  }

  flat_dataset_t ds{};
  if (!flatten_batches_feature_(batches, mode, &ds))
    return false;
  return eval_linear_gaussian_from_dataset_(model, ds, cfg_.gaussian_var_min,
                                            out_bits, out_weight, error);
}

[[nodiscard]] bool
eval_residual_linear_bits_(const linear_gaussian_t &stats_model,
                           const linear_gaussian_t &residual_model,
                           const std::vector<const train_batch_t *> &batches,
                           double *out_bits, double *out_weight,
                           std::string *error) const {
  if (error)
    error->clear();
  if (out_bits)
    *out_bits = std::numeric_limits<double>::quiet_NaN();
  if (out_weight)
    *out_weight = 0.0;
  if (!stats_model.valid || !residual_model.valid)
    return false;

  flat_dataset_t ds_enc{};
  flat_dataset_t ds_stats{};
  if (!flatten_batches_(batches, &ds_enc) ||
      !flatten_batches_stats_(batches, &ds_stats)) {
    return false;
  }
  if (!ds_enc.x.defined() || !ds_stats.x.defined())
    return false;
  if (ds_enc.x.size(0) != ds_stats.x.size(0) ||
      ds_enc.y.size(0) != ds_stats.y.size(0) ||
      ds_enc.w.size(0) != ds_stats.w.size(0)) {
    if (error)
      *error = "residual evaluation dataset alignment mismatch";
    return false;
  }

  try {
    auto y_base = predict_linear_from_dataset_(stats_model, ds_stats.x);
    auto y_res = predict_linear_from_dataset_(residual_model, ds_enc.x);
    auto y_pred = y_base + y_res;
    auto resid = ds_enc.y - y_pred;
    auto w = ds_enc.w.clamp_min(0.0);
    const double w_sum = w.sum().item<double>();
    if (!std::isfinite(w_sum) || w_sum <= 0.0)
      return false;
    auto w_col = w.unsqueeze(1);
    const double denom =
        std::max<double>(w_sum * static_cast<double>(ds_enc.y.size(1)), 1.0);
    const double mse = ((resid * resid) * w_col).sum().item<double>() / denom;
    constexpr double kTwoPi = 6.28318530717958647692;
    constexpr double kInvLn2 = 1.4426950408889634074;
    const double sigma2 =
        std::max(residual_model.sigma2, cfg_.gaussian_var_min);
    const double nll_bits =
        0.5 * (std::log(kTwoPi * sigma2) + (mse / sigma2)) * kInvLn2;
    if (out_bits)
      *out_bits = nll_bits;
    if (out_weight)
      *out_weight = denom;
    return std::isfinite(nll_bits);
  } catch (const std::exception &e) {
    if (error)
      *error = e.what();
    return false;
  }
}

void run_epoch_training_cycle_() {
  epoch_matrix_ = matrix_results_t{};
  if (epoch_batches_.empty())
    return;

  std::vector<const train_batch_t *> train{};
  std::vector<const train_batch_t *> val{};
  std::vector<const train_batch_t *> test{};
  split_epoch_batches_(&train, &val, &test);
  if (train.empty())
    train = test;
  if (train.empty())
    train = val;
  if (test.empty())
    test = val;
  if (test.empty())
    test = train;
  std::vector<const train_batch_t *> train_val = train;
  train_val.insert(train_val.end(), val.begin(), val.end());
  if (train_val.empty())
    train_val = train;

  // Prequential paths should operate at sample granularity, not batch
  // granularity.
  std::vector<train_batch_t> train_samples_store{};
  std::vector<train_batch_t> val_samples_store{};
  std::vector<train_batch_t> test_samples_store{};
  explode_batches_by_sample_(train, &train_samples_store);
  explode_batches_by_sample_(val, &val_samples_store);
  explode_batches_by_sample_(test, &test_samples_store);

  auto train_s = ptr_view_(train_samples_store);
  auto val_s = ptr_view_(val_samples_store);
  auto test_s = ptr_view_(test_samples_store);
  if (train_s.empty())
    train_s = test_s;
  if (train_s.empty())
    train_s = val_s;
  if (test_s.empty())
    test_s = val_s;
  if (test_s.empty())
    test_s = train_s;
  std::vector<const train_batch_t *> train_val_s = train_s;
  train_val_s.insert(train_val_s.end(), val_s.begin(), val_s.end());
  if (train_val_s.empty())
    train_val_s = train_s;

  target_norm_t mdn_norm{};
  const target_norm_t *mdn_norm_ptr = nullptr;
  if (fit_target_norm_(train_val_s, &mdn_norm, nullptr) ||
      fit_target_norm_(train_s, &mdn_norm, nullptr)) {
    mdn_norm_ptr = &mdn_norm;
  }

  // forecast.null
  gaussian_baseline_t null_fit{};
  if (fit_null_baseline_(train_s, &null_fit, nullptr)) {
    const auto blocks =
        make_prequential_blocks_(train_s, cfg_.prequential_blocks);
    std::vector<double> block_supports(blocks.size(), 0.0);
    for (std::size_t i = 0; i < blocks.size(); ++i) {
      (void)support_elements_(blocks[i], &block_supports[i]);
    }
    double effort_sum = 0.0;
    double effort_w = 0.0;
    double prefix_support = 0.0;
    for (std::size_t i = 1; i < blocks.size(); ++i) {
      prefix_support += block_supports[i - 1];
      std::vector<const train_batch_t *> prefix{};
      append_prefix_blocks_(blocks, i, &prefix);
      gaussian_baseline_t prefix_fit{};
      if (!fit_null_baseline_(prefix, &prefix_fit, nullptr))
        continue;
      double bits = std::numeric_limits<double>::quiet_NaN();
      double w = 0.0;
      if (!eval_null_baseline_bits_(prefix_fit, blocks[i], &bits, &w, nullptr))
        continue;
      record_prequential_row_("forecast_null", i, prefix_support, w, bits,
                              bits);
      if (std::isfinite(bits) && w > 0.0) {
        effort_sum += bits * w;
        effort_w += w;
      }
    }
    if (effort_w > 0.0) {
      epoch_matrix_.forecast_null.effort = effort_sum / effort_w;
      epoch_matrix_.forecast_null.has_effort = true;
    }
    gaussian_baseline_t final_fit{};
    if (fit_null_baseline_(train_val_s, &final_fit, nullptr)) {
      double bits = std::numeric_limits<double>::quiet_NaN();
      double w = 0.0;
      if (eval_null_baseline_bits_(final_fit, test_s, &bits, &w, nullptr) &&
          std::isfinite(bits)) {
        epoch_matrix_.forecast_null.error = bits;
        epoch_matrix_.forecast_null.has_error = true;
        if (w > 0.0 && std::isfinite(w)) {
          epoch_matrix_.forecast_null.support = w;
          epoch_matrix_.forecast_null.has_support = true;
        }
      }
    }
  }

  // forecast.stats_only (simple side-feature baseline)
  linear_gaussian_t stats_only_fit{};
  if (fit_stats_linear_gaussian_(train_val_s, &stats_only_fit, nullptr)) {
    double bits = std::numeric_limits<double>::quiet_NaN();
    double w = 0.0;
    if (eval_stats_linear_gaussian_bits_(stats_only_fit, test_s, &bits, &w,
                                         nullptr) &&
        std::isfinite(bits)) {
      epoch_matrix_.forecast_stats_only.error = bits;
      epoch_matrix_.forecast_stats_only.has_error = true;
      if (w > 0.0 && std::isfinite(w)) {
        epoch_matrix_.forecast_stats_only.support = w;
        epoch_matrix_.forecast_stats_only.has_support = true;
      }
    }
  }

  // forecast.raw_linear
  linear_gaussian_t raw_linear_fit{};
  std::vector<std::pair<double, double>> raw_linear_skill_curve{};
  if (fit_raw_linear_gaussian_(train_s, &raw_linear_fit, nullptr)) {
    const auto blocks =
        make_prequential_blocks_(train_s, cfg_.prequential_blocks);
    std::vector<double> block_supports(blocks.size(), 0.0);
    for (std::size_t i = 0; i < blocks.size(); ++i) {
      (void)support_elements_(blocks[i], &block_supports[i]);
    }
    double effort_sum = 0.0;
    double effort_w = 0.0;
    double prefix_support = 0.0;
    for (std::size_t i = 1; i < blocks.size(); ++i) {
      prefix_support += block_supports[i - 1];
      std::vector<const train_batch_t *> prefix{};
      append_prefix_blocks_(blocks, i, &prefix);
      linear_gaussian_t prefix_fit{};
      if (!fit_raw_linear_gaussian_(prefix, &prefix_fit, nullptr))
        continue;
      double row_bits = std::numeric_limits<double>::quiet_NaN();
      double w = 0.0;
      if (!eval_raw_linear_gaussian_bits_(prefix_fit, blocks[i], &row_bits, &w,
                                          nullptr)) {
        continue;
      }
      if (std::isfinite(row_bits) && w > 0.0) {
        effort_sum += row_bits * w;
        effort_w += w;
      }
      gaussian_baseline_t prefix_null{};
      double null_bits = std::numeric_limits<double>::quiet_NaN();
      if (fit_null_baseline_(prefix, &prefix_null, nullptr) &&
          eval_null_baseline_bits_(prefix_null, blocks[i], &null_bits, nullptr,
                                   nullptr) &&
          std::isfinite(null_bits) && std::isfinite(row_bits)) {
        raw_linear_skill_curve.emplace_back(prefix_support,
                                            null_bits - row_bits);
      }
      record_prequential_row_("forecast_raw_linear", i, prefix_support, w,
                              row_bits, null_bits);
    }
    if (effort_w > 0.0) {
      epoch_matrix_.forecast_raw_linear.effort = effort_sum / effort_w;
      epoch_matrix_.forecast_raw_linear.has_effort = true;
    }
    linear_gaussian_t final_fit{};
    if (fit_raw_linear_gaussian_(train_val_s, &final_fit, nullptr)) {
      double bits = std::numeric_limits<double>::quiet_NaN();
      double w = 0.0;
      if (eval_raw_linear_gaussian_bits_(final_fit, test_s, &bits, &w,
                                         nullptr) &&
          std::isfinite(bits)) {
        epoch_matrix_.forecast_raw_linear.error = bits;
        epoch_matrix_.forecast_raw_linear.has_error = true;
        if (w > 0.0 && std::isfinite(w)) {
          epoch_matrix_.forecast_raw_linear.support = w;
          epoch_matrix_.forecast_raw_linear.has_support = true;
        }
      }
    }
  }

  // forecast.linear
  linear_gaussian_t linear_fit{};
  std::vector<std::pair<double, double>> linear_skill_curve{};
  if (fit_linear_gaussian_(train_s, &linear_fit, nullptr)) {
    const auto blocks =
        make_prequential_blocks_(train_s, cfg_.prequential_blocks);
    std::vector<double> block_supports(blocks.size(), 0.0);
    for (std::size_t i = 0; i < blocks.size(); ++i) {
      (void)support_elements_(blocks[i], &block_supports[i]);
    }
    double effort_sum = 0.0;
    double effort_w = 0.0;
    double prefix_support = 0.0;
    for (std::size_t i = 1; i < blocks.size(); ++i) {
      prefix_support += block_supports[i - 1];
      std::vector<const train_batch_t *> prefix{};
      append_prefix_blocks_(blocks, i, &prefix);
      linear_gaussian_t prefix_fit{};
      if (!fit_linear_gaussian_(prefix, &prefix_fit, nullptr))
        continue;
      double row_bits = std::numeric_limits<double>::quiet_NaN();
      double w = 0.0;
      if (!eval_linear_gaussian_bits_(prefix_fit, blocks[i], &row_bits, &w,
                                      nullptr)) {
        continue;
      }
      if (std::isfinite(row_bits) && w > 0.0) {
        effort_sum += row_bits * w;
        effort_w += w;
      }
      gaussian_baseline_t prefix_null{};
      double null_bits = std::numeric_limits<double>::quiet_NaN();
      if (fit_null_baseline_(prefix, &prefix_null, nullptr) &&
          eval_null_baseline_bits_(prefix_null, blocks[i], &null_bits, nullptr,
                                   nullptr) &&
          std::isfinite(null_bits) && std::isfinite(row_bits)) {
        linear_skill_curve.emplace_back(prefix_support, null_bits - row_bits);
      }
      record_prequential_row_("forecast_linear", i, prefix_support, w, row_bits,
                              null_bits);
    }
    if (effort_w > 0.0) {
      epoch_matrix_.forecast_linear.effort = effort_sum / effort_w;
      epoch_matrix_.forecast_linear.has_effort = true;
    }
    linear_gaussian_t final_fit{};
    if (fit_linear_gaussian_(train_val_s, &final_fit, nullptr)) {
      double bits = std::numeric_limits<double>::quiet_NaN();
      double w = 0.0;
      if (eval_linear_gaussian_bits_(final_fit, test_s, &bits, &w, nullptr) &&
          std::isfinite(bits)) {
        epoch_matrix_.forecast_linear.error = bits;
        epoch_matrix_.forecast_linear.has_error = true;
        if (w > 0.0 && std::isfinite(w)) {
          epoch_matrix_.forecast_linear.support = w;
          epoch_matrix_.forecast_linear.has_support = true;
        }
      }
    }
  }

  // forecast.residual.linear (stats baseline + encoding residual head)
  linear_gaussian_t stats_model{};
  linear_gaussian_t residual_model{};
  if (fit_stats_linear_gaussian_(train_val_s, &stats_model, nullptr)) {
    flat_dataset_t ds_enc_train{};
    flat_dataset_t ds_stats_train{};
    if (flatten_batches_(train_val_s, &ds_enc_train) &&
        flatten_batches_stats_(train_val_s, &ds_stats_train) &&
        ds_enc_train.x.size(0) == ds_stats_train.x.size(0) &&
        ds_enc_train.y.size(0) == ds_stats_train.y.size(0) &&
        ds_enc_train.w.size(0) == ds_stats_train.w.size(0)) {
      auto y_base_train =
          predict_linear_from_dataset_(stats_model, ds_stats_train.x);
      flat_dataset_t ds_res_train{};
      ds_res_train.x = ds_enc_train.x;
      ds_res_train.y = ds_enc_train.y - y_base_train;
      ds_res_train.w = ds_enc_train.w;
      if (fit_linear_gaussian_from_dataset_(
              ds_res_train, cfg_.linear_ridge_lambda, cfg_.gaussian_var_min,
              &residual_model, nullptr)) {
        double bits = std::numeric_limits<double>::quiet_NaN();
        double w = 0.0;
        if (eval_residual_linear_bits_(stats_model, residual_model, test_s,
                                       &bits, &w, nullptr) &&
            std::isfinite(bits)) {
          epoch_matrix_.forecast_residual_linear.error = bits;
          epoch_matrix_.forecast_residual_linear.has_error = true;
          if (w > 0.0 && std::isfinite(w)) {
            epoch_matrix_.forecast_residual_linear.support = w;
            epoch_matrix_.forecast_residual_linear.has_support = true;
          }
        }
      }
    }
  }

  // forecast.raw_mdn prequential effort + skill curve
  std::vector<std::pair<double, double>> raw_mdn_skill_curve{};
  const auto raw_input_dims =
      feature_dims_from_batches_(train_val_s, feature_mode_e::Raw);
  if (raw_input_dims > 0) {
    try {
      cuwacunu::wikimyei::mdn::MdnModel raw_semantic_model{nullptr};
      std::unique_ptr<torch::optim::Optimizer> raw_optimizer{};
      initialize_runtime_model_for_input_dims_or_throw_(
          raw_input_dims, &raw_semantic_model, &raw_optimizer);
      const auto blocks =
          make_prequential_blocks_(train_s, cfg_.prequential_blocks);
      std::vector<double> block_supports(blocks.size(), 0.0);
      for (std::size_t i = 0; i < blocks.size(); ++i) {
        (void)support_elements_(blocks[i], &block_supports[i]);
      }

      double effort_sum = 0.0;
      double effort_w = 0.0;
      double prefix_support = 0.0;
      std::vector<const train_batch_t *> prefix{};
      for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (i > 0) {
          double row_bits = std::numeric_limits<double>::quiet_NaN();
          double w = 0.0;
          if (evaluate_mdn_batches_for_model_(blocks[i], feature_mode_e::Raw,
                                              mdn_norm_ptr, raw_semantic_model,
                                              &row_bits, &w, nullptr) &&
              std::isfinite(row_bits) && w > 0.0) {
            effort_sum += row_bits * w;
            effort_w += w;
          }
          gaussian_baseline_t prefix_null{};
          double null_bits = std::numeric_limits<double>::quiet_NaN();
          if (fit_null_baseline_(prefix, &prefix_null, nullptr) &&
              eval_null_baseline_bits_(prefix_null, blocks[i], &null_bits,
                                       nullptr, nullptr) &&
              std::isfinite(null_bits) && std::isfinite(row_bits)) {
            raw_mdn_skill_curve.emplace_back(prefix_support,
                                             null_bits - row_bits);
          }
          record_prequential_row_("forecast_raw_mdn", i, prefix_support, w,
                                  row_bits, null_bits);
        }
        train_mdn_batches_for_model_(blocks[i], feature_mode_e::Raw,
                                     mdn_norm_ptr, raw_semantic_model,
                                     raw_optimizer.get(),
                                     /*record_training_stats=*/false);
        prefix.insert(prefix.end(), blocks[i].begin(), blocks[i].end());
        prefix_support += block_supports[i];
      }
      if (effort_w > 0.0) {
        epoch_matrix_.forecast_raw_mdn.effort = effort_sum / effort_w;
        epoch_matrix_.forecast_raw_mdn.has_effort = true;
      }
    } catch (const std::exception &) {
      // Raw-surface MDN is auxiliary; keep the main evaluator alive if it
      // fails.
    }

    // forecast.raw_mdn held-out error
    try {
      cuwacunu::wikimyei::mdn::MdnModel raw_semantic_model{nullptr};
      std::unique_ptr<torch::optim::Optimizer> raw_optimizer{};
      initialize_runtime_model_for_input_dims_or_throw_(
          raw_input_dims, &raw_semantic_model, &raw_optimizer);
      train_mdn_batches_for_model_(train_val_s, feature_mode_e::Raw,
                                   mdn_norm_ptr, raw_semantic_model,
                                   raw_optimizer.get(),
                                   /*record_training_stats=*/false);
      double err_bits = std::numeric_limits<double>::quiet_NaN();
      double err_w = 0.0;
      if (evaluate_mdn_batches_for_model_(test_s, feature_mode_e::Raw,
                                          mdn_norm_ptr, raw_semantic_model,
                                          &err_bits, &err_w, nullptr) &&
          std::isfinite(err_bits)) {
        epoch_matrix_.forecast_raw_mdn.error = err_bits;
        epoch_matrix_.forecast_raw_mdn.has_error = true;
        if (err_w > 0.0 && std::isfinite(err_w)) {
          epoch_matrix_.forecast_raw_mdn.support = err_w;
          epoch_matrix_.forecast_raw_mdn.has_support = true;
        }
      }
    } catch (const std::exception &) {
      // Raw-surface MDN is auxiliary; keep the main evaluator alive if it
      // fails.
    }
  }

  // forecast.mdn prequential effort + skill curve
  std::vector<std::pair<double, double>> mdn_skill_curve{};
  try {
    initialize_runtime_model_or_throw_();
    const auto blocks =
        make_prequential_blocks_(train_s, cfg_.prequential_blocks);
    std::vector<double> block_supports(blocks.size(), 0.0);
    for (std::size_t i = 0; i < blocks.size(); ++i) {
      (void)support_elements_(blocks[i], &block_supports[i]);
    }

    double effort_sum = 0.0;
    double effort_w = 0.0;
    double prefix_support = 0.0;
    std::vector<const train_batch_t *> prefix{};
    for (std::size_t i = 0; i < blocks.size(); ++i) {
      if (i > 0) {
        double row_bits = std::numeric_limits<double>::quiet_NaN();
        double w = 0.0;
        if (evaluate_mdn_batches_(blocks[i], mdn_norm_ptr, &row_bits, &w,
                                  nullptr) &&
            std::isfinite(row_bits) && w > 0.0) {
          effort_sum += row_bits * w;
          effort_w += w;
        }
        gaussian_baseline_t prefix_null{};
        double null_bits = std::numeric_limits<double>::quiet_NaN();
        if (fit_null_baseline_(prefix, &prefix_null, nullptr) &&
            eval_null_baseline_bits_(prefix_null, blocks[i], &null_bits,
                                     nullptr, nullptr) &&
            std::isfinite(null_bits) && std::isfinite(row_bits)) {
          mdn_skill_curve.emplace_back(prefix_support, null_bits - row_bits);
        }
        record_prequential_row_("forecast_mdn", i, prefix_support, w, row_bits,
                                null_bits);
      }
      train_mdn_batches_(blocks[i], mdn_norm_ptr);
      prefix.insert(prefix.end(), blocks[i].begin(), blocks[i].end());
      prefix_support += block_supports[i];
    }
    if (effort_w > 0.0) {
      epoch_matrix_.forecast_mdn.effort = effort_sum / effort_w;
      epoch_matrix_.forecast_mdn.has_effort = true;
    }
  } catch (const std::exception &) {
    ++epoch_.optimizer_failures;
  }

  // forecast.mdn held-out error + health
  try {
    initialize_runtime_model_or_throw_();
    double cold_bits = std::numeric_limits<double>::quiet_NaN();
    if (evaluate_mdn_batches_(test_s, mdn_norm_ptr, &cold_bits, nullptr,
                              nullptr) &&
        std::isfinite(cold_bits)) {
      epoch_matrix_.cold_start_loss = cold_bits;
      epoch_matrix_.has_cold_start_loss = true;
    }
    train_mdn_batches_(train_val_s, mdn_norm_ptr);
    double train_fit_bits = std::numeric_limits<double>::quiet_NaN();
    if (evaluate_mdn_batches_(train_val_s, mdn_norm_ptr, &train_fit_bits,
                              nullptr, nullptr) &&
        std::isfinite(train_fit_bits)) {
      epoch_matrix_.train_fit_loss = train_fit_bits;
      epoch_matrix_.has_train_fit_loss = true;
    }
    double err_bits = std::numeric_limits<double>::quiet_NaN();
    double err_w = 0.0;
    if (evaluate_mdn_batches_(test_s, mdn_norm_ptr, &err_bits, &err_w,
                              nullptr) &&
        std::isfinite(err_bits)) {
      epoch_matrix_.forecast_mdn.error = err_bits;
      epoch_matrix_.forecast_mdn.has_error = true;
      if (err_w > 0.0 && std::isfinite(err_w)) {
        epoch_matrix_.forecast_mdn.support = err_w;
        epoch_matrix_.forecast_mdn.has_support = true;
      }
    }
  } catch (const std::exception &) {
    ++epoch_.optimizer_failures;
  }

  if (epoch_matrix_.has_train_fit_loss &&
      epoch_matrix_.forecast_mdn.has_error) {
    epoch_matrix_.generalization_gap =
        epoch_matrix_.train_fit_loss - epoch_matrix_.forecast_mdn.error;
    epoch_matrix_.has_generalization_gap =
        std::isfinite(epoch_matrix_.generalization_gap);
  }

  // Skills relative to forecast.null.
  if (epoch_matrix_.forecast_null.has_effort &&
      epoch_matrix_.forecast_mdn.has_effort) {
    epoch_matrix_.forecast_null.effort_skill = 0.0;
    epoch_matrix_.forecast_null.has_effort_skill = true;
    epoch_matrix_.forecast_mdn.effort_skill =
        epoch_matrix_.forecast_null.effort - epoch_matrix_.forecast_mdn.effort;
    epoch_matrix_.forecast_mdn.has_effort_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_effort &&
      epoch_matrix_.forecast_raw_mdn.has_effort) {
    epoch_matrix_.forecast_null.effort_skill = 0.0;
    epoch_matrix_.forecast_null.has_effort_skill = true;
    epoch_matrix_.forecast_raw_mdn.effort_skill =
        epoch_matrix_.forecast_null.effort -
        epoch_matrix_.forecast_raw_mdn.effort;
    epoch_matrix_.forecast_raw_mdn.has_effort_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_effort &&
      epoch_matrix_.forecast_linear.has_effort) {
    epoch_matrix_.forecast_null.effort_skill = 0.0;
    epoch_matrix_.forecast_null.has_effort_skill = true;
    epoch_matrix_.forecast_linear.effort_skill =
        epoch_matrix_.forecast_null.effort -
        epoch_matrix_.forecast_linear.effort;
    epoch_matrix_.forecast_linear.has_effort_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_effort &&
      epoch_matrix_.forecast_raw_linear.has_effort) {
    epoch_matrix_.forecast_null.effort_skill = 0.0;
    epoch_matrix_.forecast_null.has_effort_skill = true;
    epoch_matrix_.forecast_raw_linear.effort_skill =
        epoch_matrix_.forecast_null.effort -
        epoch_matrix_.forecast_raw_linear.effort;
    epoch_matrix_.forecast_raw_linear.has_effort_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_error &&
      epoch_matrix_.forecast_stats_only.has_error) {
    epoch_matrix_.forecast_null.error_skill = 0.0;
    epoch_matrix_.forecast_null.has_error_skill = true;
    epoch_matrix_.forecast_stats_only.error_skill =
        epoch_matrix_.forecast_null.error -
        epoch_matrix_.forecast_stats_only.error;
    epoch_matrix_.forecast_stats_only.has_error_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_error &&
      epoch_matrix_.forecast_raw_mdn.has_error) {
    epoch_matrix_.forecast_null.error_skill = 0.0;
    epoch_matrix_.forecast_null.has_error_skill = true;
    epoch_matrix_.forecast_raw_mdn.error_skill =
        epoch_matrix_.forecast_null.error -
        epoch_matrix_.forecast_raw_mdn.error;
    epoch_matrix_.forecast_raw_mdn.has_error_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_error &&
      epoch_matrix_.forecast_mdn.has_error) {
    epoch_matrix_.forecast_null.error_skill = 0.0;
    epoch_matrix_.forecast_null.has_error_skill = true;
    epoch_matrix_.forecast_mdn.error_skill =
        epoch_matrix_.forecast_null.error - epoch_matrix_.forecast_mdn.error;
    epoch_matrix_.forecast_mdn.has_error_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_error &&
      epoch_matrix_.forecast_raw_linear.has_error) {
    epoch_matrix_.forecast_null.error_skill = 0.0;
    epoch_matrix_.forecast_null.has_error_skill = true;
    epoch_matrix_.forecast_raw_linear.error_skill =
        epoch_matrix_.forecast_null.error -
        epoch_matrix_.forecast_raw_linear.error;
    epoch_matrix_.forecast_raw_linear.has_error_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_error &&
      epoch_matrix_.forecast_linear.has_error) {
    epoch_matrix_.forecast_null.error_skill = 0.0;
    epoch_matrix_.forecast_null.has_error_skill = true;
    epoch_matrix_.forecast_linear.error_skill =
        epoch_matrix_.forecast_null.error - epoch_matrix_.forecast_linear.error;
    epoch_matrix_.forecast_linear.has_error_skill = true;
  }
  if (epoch_matrix_.forecast_null.has_error &&
      epoch_matrix_.forecast_residual_linear.has_error) {
    epoch_matrix_.forecast_null.error_skill = 0.0;
    epoch_matrix_.forecast_null.has_error_skill = true;
    epoch_matrix_.forecast_residual_linear.error_skill =
        epoch_matrix_.forecast_null.error -
        epoch_matrix_.forecast_residual_linear.error;
    epoch_matrix_.forecast_residual_linear.has_error_skill = true;
  }

  // n90 from prequential skill curves.
  if (epoch_matrix_.forecast_linear.has_error_skill &&
      epoch_matrix_.forecast_linear.error_skill > 0.0) {
    (void)apply_n90_from_curve_(linear_skill_curve,
                                epoch_matrix_.forecast_linear.error_skill,
                                &epoch_matrix_.forecast_linear);
  }
  if (epoch_matrix_.forecast_raw_linear.has_error_skill &&
      epoch_matrix_.forecast_raw_linear.error_skill > 0.0) {
    (void)apply_n90_from_curve_(raw_linear_skill_curve,
                                epoch_matrix_.forecast_raw_linear.error_skill,
                                &epoch_matrix_.forecast_raw_linear);
  }
  if (epoch_matrix_.forecast_raw_mdn.has_error_skill &&
      epoch_matrix_.forecast_raw_mdn.error_skill > 0.0) {
    (void)apply_n90_from_curve_(raw_mdn_skill_curve,
                                epoch_matrix_.forecast_raw_mdn.error_skill,
                                &epoch_matrix_.forecast_raw_mdn);
  }
  if (epoch_matrix_.forecast_mdn.has_error_skill &&
      epoch_matrix_.forecast_mdn.error_skill > 0.0) {
    (void)apply_n90_from_curve_(mdn_skill_curve,
                                epoch_matrix_.forecast_mdn.error_skill,
                                &epoch_matrix_.forecast_mdn);
  }

  // Selectivity controls for linear/mdn using block-shuffled targets.
  if (!train_val.empty() && !test.empty()) {
    const std::uint64_t base_seed = static_cast<std::uint64_t>(
        std::max<std::int64_t>(0, cfg_.control_shuffle_seed));
    auto ctrl_train_val_store = make_block_shuffled_control_(
        train_val, cfg_.control_shuffle_block, base_seed + 17ULL);
    auto ctrl_test_store = make_block_shuffled_control_(
        test, cfg_.control_shuffle_block, base_seed + 29ULL);
    auto ctrl_train_val = ptr_view_(ctrl_train_val_store);
    auto ctrl_test = ptr_view_(ctrl_test_store);

    // linear selectivity
    linear_gaussian_t ctrl_linear{};
    gaussian_baseline_t ctrl_null{};
    double ctrl_null_bits = std::numeric_limits<double>::quiet_NaN();
    double ctrl_linear_bits = std::numeric_limits<double>::quiet_NaN();
    if (fit_null_baseline_(ctrl_train_val, &ctrl_null, nullptr) &&
        eval_null_baseline_bits_(ctrl_null, ctrl_test, &ctrl_null_bits, nullptr,
                                 nullptr) &&
        fit_linear_gaussian_(ctrl_train_val, &ctrl_linear, nullptr) &&
        eval_linear_gaussian_bits_(ctrl_linear, ctrl_test, &ctrl_linear_bits,
                                   nullptr, nullptr) &&
        std::isfinite(ctrl_null_bits) && std::isfinite(ctrl_linear_bits) &&
        epoch_matrix_.forecast_linear.has_error_skill &&
        epoch_matrix_.forecast_linear.error_skill > 0.0) {
      const double ctrl_skill = ctrl_null_bits - ctrl_linear_bits;
      epoch_matrix_.forecast_linear.selectivity =
          epoch_matrix_.forecast_linear.error_skill - ctrl_skill;
      epoch_matrix_.forecast_linear.has_selectivity =
          std::isfinite(epoch_matrix_.forecast_linear.selectivity);
    }

    // raw linear selectivity
    linear_gaussian_t ctrl_raw_linear{};
    double ctrl_raw_linear_bits = std::numeric_limits<double>::quiet_NaN();
    if (fit_null_baseline_(ctrl_train_val, &ctrl_null, nullptr) &&
        eval_null_baseline_bits_(ctrl_null, ctrl_test, &ctrl_null_bits, nullptr,
                                 nullptr) &&
        fit_raw_linear_gaussian_(ctrl_train_val, &ctrl_raw_linear, nullptr) &&
        eval_raw_linear_gaussian_bits_(ctrl_raw_linear, ctrl_test,
                                       &ctrl_raw_linear_bits, nullptr,
                                       nullptr) &&
        std::isfinite(ctrl_null_bits) && std::isfinite(ctrl_raw_linear_bits) &&
        epoch_matrix_.forecast_raw_linear.has_error_skill &&
        epoch_matrix_.forecast_raw_linear.error_skill > 0.0) {
      const double ctrl_skill = ctrl_null_bits - ctrl_raw_linear_bits;
      epoch_matrix_.forecast_raw_linear.selectivity =
          epoch_matrix_.forecast_raw_linear.error_skill - ctrl_skill;
      epoch_matrix_.forecast_raw_linear.has_selectivity =
          std::isfinite(epoch_matrix_.forecast_raw_linear.selectivity);
    }

    // mdn selectivity
    double ctrl_mdn_bits = std::numeric_limits<double>::quiet_NaN();
    double ctrl_null_bits_mdn = std::numeric_limits<double>::quiet_NaN();
    if (fit_null_baseline_(ctrl_train_val, &ctrl_null, nullptr) &&
        eval_null_baseline_bits_(ctrl_null, ctrl_test, &ctrl_null_bits_mdn,
                                 nullptr, nullptr) &&
        std::isfinite(ctrl_null_bits_mdn) &&
        epoch_matrix_.forecast_mdn.has_error_skill &&
        epoch_matrix_.forecast_mdn.error_skill > 0.0) {
      const auto saved_steps = epoch_.train_steps;
      const auto saved_failures = epoch_.optimizer_failures;
      const auto saved_train_attempted = epoch_.train_batches_attempted;
      const auto saved_train_positions = epoch_.train_ingested_positions;
      const auto saved_train_support = epoch_.train_ingested_support;
      try {
        initialize_runtime_model_or_throw_();
        train_mdn_batches_(ctrl_train_val, mdn_norm_ptr);
        if (evaluate_mdn_batches_(ctrl_test, mdn_norm_ptr, &ctrl_mdn_bits,
                                  nullptr, nullptr) &&
            std::isfinite(ctrl_mdn_bits)) {
          const double ctrl_skill = ctrl_null_bits_mdn - ctrl_mdn_bits;
          epoch_matrix_.forecast_mdn.selectivity =
              epoch_matrix_.forecast_mdn.error_skill - ctrl_skill;
          epoch_matrix_.forecast_mdn.has_selectivity =
              std::isfinite(epoch_matrix_.forecast_mdn.selectivity);
        }
      } catch (const std::exception &) {
        // no-op
      }
      epoch_.train_steps = saved_steps;
      epoch_.optimizer_failures = saved_failures;
      epoch_.train_batches_attempted = saved_train_attempted;
      epoch_.train_ingested_positions = saved_train_positions;
      epoch_.train_ingested_support = saved_train_support;
    }

    // raw mdn selectivity
    double ctrl_raw_mdn_bits = std::numeric_limits<double>::quiet_NaN();
    if (fit_null_baseline_(ctrl_train_val, &ctrl_null, nullptr) &&
        eval_null_baseline_bits_(ctrl_null, ctrl_test, &ctrl_null_bits_mdn,
                                 nullptr, nullptr) &&
        std::isfinite(ctrl_null_bits_mdn) &&
        epoch_matrix_.forecast_raw_mdn.has_error_skill &&
        epoch_matrix_.forecast_raw_mdn.error_skill > 0.0) {
      try {
        const auto ctrl_raw_input_dims =
            feature_dims_from_batches_(ctrl_train_val, feature_mode_e::Raw);
        if (ctrl_raw_input_dims > 0) {
          cuwacunu::wikimyei::mdn::MdnModel ctrl_raw_model{nullptr};
          std::unique_ptr<torch::optim::Optimizer> ctrl_raw_optimizer{};
          initialize_runtime_model_for_input_dims_or_throw_(
              ctrl_raw_input_dims, &ctrl_raw_model, &ctrl_raw_optimizer);
          train_mdn_batches_for_model_(ctrl_train_val, feature_mode_e::Raw,
                                       mdn_norm_ptr, ctrl_raw_model,
                                       ctrl_raw_optimizer.get(),
                                       /*record_training_stats=*/false);
          if (evaluate_mdn_batches_for_model_(
                  ctrl_test, feature_mode_e::Raw, mdn_norm_ptr, ctrl_raw_model,
                  &ctrl_raw_mdn_bits, nullptr, nullptr) &&
              std::isfinite(ctrl_raw_mdn_bits)) {
            const double ctrl_skill = ctrl_null_bits_mdn - ctrl_raw_mdn_bits;
            epoch_matrix_.forecast_raw_mdn.selectivity =
                epoch_matrix_.forecast_raw_mdn.error_skill - ctrl_skill;
            epoch_matrix_.forecast_raw_mdn.has_selectivity =
                std::isfinite(epoch_matrix_.forecast_raw_mdn.selectivity);
          }
        }
      } catch (const std::exception &) {
        // no-op
      }
    }
  }
}
