// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

enum class cell_valid_policy_t {
  any_feature,
  all_features,
  required_features,
  min_valid_fraction,
};

struct channel_preserving_encoder_options_t {
  int64_t channel_count{0};
  int64_t history_length{0};
  int64_t input_width{0};
  int64_t encoding_dim{0};
  int64_t feature_hidden_dim{64};
  int64_t temporal_depth{1};
  double recency_decay{0.85};
  cell_valid_policy_t cell_valid_policy{cell_valid_policy_t::all_features};
  std::vector<int64_t> required_feature_coords{};
  double min_valid_fraction{1.0};
  bool use_missingness_indicators{true};
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};
};

struct channel_preserving_encoder_output_t {
  torch::Tensor sequence{};        // [M,C,Hx,De]
  torch::Tensor sequence_mask{};   // [M,C,Hx], bool
  torch::Tensor reduced{};         // [M,C,De]
  torch::Tensor reduced_mask{};    // [M,C], bool
  torch::Tensor reducer_weights{}; // [M,C,Hx]
};

namespace channel_preserving_encoder_detail {

inline void validate_options(const channel_preserving_encoder_options_t &opts) {
  if (opts.channel_count <= 0 || opts.history_length <= 0 ||
      opts.input_width <= 0 || opts.encoding_dim <= 0 ||
      opts.feature_hidden_dim <= 0 || opts.temporal_depth < 0) {
    throw std::runtime_error(
        "[channel_preserving_encoder] invalid architecture dimensions");
  }
  if (!(opts.recency_decay > 0.0 && opts.recency_decay <= 1.0) ||
      !std::isfinite(opts.recency_decay)) {
    throw std::runtime_error(
        "[channel_preserving_encoder] recency_decay must be in (0,1]");
  }
  if (!(opts.min_valid_fraction > 0.0 && opts.min_valid_fraction <= 1.0) ||
      !std::isfinite(opts.min_valid_fraction)) {
    throw std::runtime_error(
        "[channel_preserving_encoder] min_valid_fraction must be in (0,1]");
  }
  if (opts.cell_valid_policy == cell_valid_policy_t::required_features) {
    if (opts.required_feature_coords.empty()) {
      throw std::runtime_error(
          "[channel_preserving_encoder] required feature coords are empty");
    }
    std::unordered_set<int64_t> seen;
    seen.reserve(opts.required_feature_coords.size());
    for (const int64_t coord : opts.required_feature_coords) {
      if (coord < 0 || coord >= opts.input_width) {
        throw std::runtime_error(
            "[channel_preserving_encoder] required feature coord out of range");
      }
      if (!seen.insert(coord).second) {
        throw std::runtime_error(
            "[channel_preserving_encoder] duplicate required feature coord");
      }
    }
  }
}

inline void validate_input(const torch::Tensor &data,
                           const torch::Tensor &feature_mask,
                           const channel_preserving_encoder_options_t &opts) {
  TORCH_CHECK(data.defined() && data.dim() == 4,
              "[channel_preserving_encoder] data must be [M,C,Hx,Dx]");
  TORCH_CHECK(data.is_floating_point(),
              "[channel_preserving_encoder] data must be floating point");
  TORCH_CHECK(data.size(1) == opts.channel_count &&
                  data.size(2) == opts.history_length &&
                  data.size(3) == opts.input_width,
              "[channel_preserving_encoder] data shape does not match options");
  if (feature_mask.defined()) {
    TORCH_CHECK(feature_mask.dim() == 4,
                "[channel_preserving_encoder] feature_mask must be "
                "[M,C,Hx,Dx]");
    TORCH_CHECK(
        feature_mask.sizes() == data.sizes(),
        "[channel_preserving_encoder] feature_mask/data shape mismatch");
  }
}

inline torch::Tensor make_feature_mask(const torch::Tensor &data,
                                       const torch::Tensor &feature_mask) {
  const auto finite = torch::isfinite(data);
  if (!feature_mask.defined()) {
    return finite;
  }
  return feature_mask
      .to(torch::TensorOptions().dtype(torch::kBool).device(data.device()))
      .logical_and(finite);
}

inline torch::Tensor
make_sequence_mask(const torch::Tensor &valid_feature,
                   const channel_preserving_encoder_options_t &opts) {
  TORCH_CHECK(valid_feature.dim() == 4,
              "[channel_preserving_encoder] valid_feature must be "
              "[M,C,Hx,Dx]");
  switch (opts.cell_valid_policy) {
  case cell_valid_policy_t::any_feature:
    return valid_feature.any(/*dim=*/3);
  case cell_valid_policy_t::all_features:
    return valid_feature.all(/*dim=*/3);
  case cell_valid_policy_t::required_features: {
    auto idx = torch::tensor(opts.required_feature_coords,
                             torch::TensorOptions()
                                 .dtype(torch::kInt64)
                                 .device(valid_feature.device()));
    return valid_feature.index_select(/*dim=*/3, idx).all(/*dim=*/3);
  }
  case cell_valid_policy_t::min_valid_fraction: {
    const auto fraction = valid_feature.to(torch::kFloat64).mean(/*dim=*/3);
    return fraction >= opts.min_valid_fraction;
  }
  }
  TORCH_CHECK(false, "[channel_preserving_encoder] unknown cell_valid_policy");
}

inline torch::Tensor masked_local_temporal_context(const torch::Tensor &x,
                                                   const torch::Tensor &mask) {
  TORCH_CHECK(x.dim() == 4, "[channel_preserving_encoder] x must be "
                            "[M,C,Hx,De]");
  TORCH_CHECK(mask.dim() == 3,
              "[channel_preserving_encoder] mask must be [M,C,Hx]");
  TORCH_CHECK(x.size(0) == mask.size(0) && x.size(1) == mask.size(1) &&
                  x.size(2) == mask.size(2),
              "[channel_preserving_encoder] x/mask shape mismatch");

  using torch::indexing::Slice;
  const auto Hx = x.size(2);
  auto out = torch::zeros_like(x);
  auto support = torch::zeros({x.size(0), x.size(1), Hx, 1}, x.options());
  const auto source_mask = mask.to(x.dtype()).unsqueeze(-1);
  const auto x_masked = x * source_mask;

  for (int64_t delta = -1; delta <= 1; ++delta) {
    const int64_t dst_begin = delta > 0 ? delta : 0;
    const int64_t src_begin = delta < 0 ? -delta : 0;
    const int64_t length = Hx - std::abs(delta);
    if (length <= 0) {
      continue;
    }
    out.index({Slice(), Slice(), Slice(dst_begin, dst_begin + length), Slice()})
        .add_(x_masked.index(
            {Slice(), Slice(), Slice(src_begin, src_begin + length), Slice()}));
    support
        .index(
            {Slice(), Slice(), Slice(dst_begin, dst_begin + length), Slice()})
        .add_(source_mask.index(
            {Slice(), Slice(), Slice(src_begin, src_begin + length), Slice()}));
  }

  const auto mixed = out / support.clamp_min(1.0);
  return mixed.masked_fill(mask.logical_not().unsqueeze(-1), 0.0);
}

inline std::pair<torch::Tensor, torch::Tensor>
anchor_weighted_reduce(const torch::Tensor &sequence,
                       const torch::Tensor &sequence_mask,
                       double recency_decay) {
  TORCH_CHECK(sequence.dim() == 4,
              "[channel_preserving_encoder] sequence must be [M,C,Hx,De]");
  TORCH_CHECK(sequence_mask.dim() == 3,
              "[channel_preserving_encoder] sequence_mask must be [M,C,Hx]");
  const auto Hx = sequence.size(2);
  auto opts = sequence.options();
  auto age = static_cast<double>(Hx - 1) -
             torch::arange(Hx, torch::TensorOptions()
                                   .dtype(sequence.dtype())
                                   .device(sequence.device()));
  auto base = torch::full({Hx}, recency_decay, opts);
  auto prior = torch::pow(base, age).view({1, 1, Hx});
  auto valid_weight = prior * sequence_mask.to(sequence.dtype());
  auto denom = valid_weight.sum(/*dim=*/2, /*keepdim=*/true);
  auto normalized =
      torch::where(denom > 0.0, valid_weight / denom.clamp_min(1e-12),
                   torch::zeros_like(valid_weight));
  auto reduced = (sequence * normalized.unsqueeze(-1)).sum(/*dim=*/2);
  auto reduced_mask = sequence_mask.any(/*dim=*/2);
  reduced = reduced.masked_fill(reduced_mask.logical_not().unsqueeze(-1), 0.0);
  return {reduced, normalized};
}

} // namespace channel_preserving_encoder_detail

class ChannelPreservingEncoderImpl : public torch::nn::Module {
public:
  explicit ChannelPreservingEncoderImpl(
      channel_preserving_encoder_options_t options)
      : options_(std::move(options)) {
    channel_preserving_encoder_detail::validate_options(options_);
    const auto feature_input_dim = options_.use_missingness_indicators
                                       ? options_.input_width * 2
                                       : options_.input_width;
    feature_in_ = register_module(
        "feature_in",
        torch::nn::Linear(feature_input_dim, options_.feature_hidden_dim));
    feature_out_ = register_module(
        "feature_out",
        torch::nn::Linear(options_.feature_hidden_dim, options_.encoding_dim));
    temporal_layers_.reserve(static_cast<std::size_t>(options_.temporal_depth));
    for (int64_t i = 0; i < options_.temporal_depth; ++i) {
      auto layer =
          torch::nn::Linear(options_.encoding_dim, options_.encoding_dim);
      temporal_layers_.push_back(register_module(
          "temporal_proj_" + std::to_string(i), std::move(layer)));
    }
    this->to(options_.device, options_.dtype);
  }

  [[nodiscard]] const channel_preserving_encoder_options_t &options() const {
    return options_;
  }

  [[nodiscard]] channel_preserving_encoder_output_t
  forward(const torch::Tensor &data,
          const torch::Tensor &feature_mask = torch::Tensor()) {
    channel_preserving_encoder_detail::validate_input(data, feature_mask,
                                                      options_);
    auto x = data.to(
        torch::TensorOptions().dtype(options_.dtype).device(options_.device));
    auto valid_feature =
        channel_preserving_encoder_detail::make_feature_mask(x, feature_mask);
    auto values = torch::where(valid_feature, x, torch::zeros_like(x));
    auto cell_input = options_.use_missingness_indicators
                          ? torch::cat({values, valid_feature.to(x.dtype())},
                                       /*dim=*/-1)
                          : values;

    const auto M = x.size(0);
    auto h = torch::gelu(feature_in_->forward(cell_input.reshape(
        {M * options_.channel_count * options_.history_length,
         cell_input.size(3)})));
    h = feature_out_->forward(h).view({M, options_.channel_count,
                                       options_.history_length,
                                       options_.encoding_dim});

    auto sequence_mask = channel_preserving_encoder_detail::make_sequence_mask(
        valid_feature, options_);
    h = h.masked_fill(sequence_mask.logical_not().unsqueeze(-1), 0.0);

    for (auto &layer : temporal_layers_) {
      auto mixed =
          channel_preserving_encoder_detail::masked_local_temporal_context(
              h, sequence_mask);
      auto update =
          torch::gelu(layer->forward(mixed.reshape(
                          {M * options_.channel_count * options_.history_length,
                           options_.encoding_dim})))
              .view_as(h);
      h = (h + update)
              .masked_fill(sequence_mask.logical_not().unsqueeze(-1), 0.0);
    }

    auto [reduced, reducer_weights] =
        channel_preserving_encoder_detail::anchor_weighted_reduce(
            h, sequence_mask, options_.recency_decay);
    channel_preserving_encoder_output_t out{};
    out.sequence = std::move(h);
    out.sequence_mask = std::move(sequence_mask);
    out.reduced = std::move(reduced);
    out.reduced_mask = out.sequence_mask.any(/*dim=*/2);
    out.reducer_weights = std::move(reducer_weights);
    return out;
  }

private:
  channel_preserving_encoder_options_t options_{};
  torch::nn::Linear feature_in_{nullptr};
  torch::nn::Linear feature_out_{nullptr};
  std::vector<torch::nn::Linear> temporal_layers_{};
};

TORCH_MODULE(ChannelPreservingEncoder);

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
