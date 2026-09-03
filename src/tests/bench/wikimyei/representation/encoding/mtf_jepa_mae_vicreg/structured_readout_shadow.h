// SPDX-License-Identifier: MIT
#pragma once

#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::representation::encoding::
    mtf_jepa_mae_vicreg::structured_readout_shadow {

// SRR-1 is deliberately test-only.  These constants describe the exact
// representation surface established by PSM-1, not a new production policy.
inline constexpr int64_t kChannelCount = 3;
inline constexpr int64_t kHistoryLength = 30;
inline constexpr int64_t kInputWidth = 9;
inline constexpr int64_t kDomainCount = 2;
inline constexpr int64_t kScaleCount = 4;
inline constexpr int64_t kTokensPerChannel = 24;
inline constexpr int64_t kTokenCount = kChannelCount * kTokensPerChannel;
inline constexpr int64_t kLatentDim = 32;
inline constexpr int64_t kCellCount = 16;
inline constexpr int64_t kProjectionInputWidth =
    kTokensPerChannel * kLatentDim;
inline constexpr int64_t kProjectionOutputWidth = kLatentDim;

inline constexpr std::array<int64_t, kScaleCount> kTokensPerScale{7, 3, 1,
                                                                  1};
inline constexpr std::array<int64_t, kScaleCount> kTimeScales{8, 16, 32, 64};
inline constexpr std::array<int64_t, kScaleCount> kScaleStrides{4, 8, 16, 32};
inline constexpr std::array<int64_t, kScaleCount> kScaleCellOffsets{0, 3, 6,
                                                                     7};
inline constexpr std::array<int64_t, kTokensPerChannel> kFrozenCellIds{
    0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  6,  7,
    8,  8,  9,  9,  9,  10, 10, 11, 12, 13, 14, 15};
inline constexpr std::array<int64_t, kCellCount> kFrozenCellCounts{
    2, 3, 2, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1, 1, 1};
inline constexpr std::array<int64_t, kTokensPerChannel> kFrozenStarts{
    0, 4, 8, 12, 16, 20, 22, 0, 8, 14, 0, 0,
    0, 4, 8, 12, 16, 20, 22, 0, 8, 14, 0, 0};
inline constexpr std::array<int64_t, kTokensPerChannel> kFrozenWidths{
    8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 30, 30,
    8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 30, 30};

struct readout_plan_t {
  // Each row maps canonical (domain, scale, start, width, source-index) order
  // back to the corresponding source token index.
  std::array<std::array<int64_t, kTokensPerChannel>, kChannelCount>
      ordered_token_indices{};
  std::array<std::array<int64_t, kTokensPerChannel>, kChannelCount>
      ordered_cell_ids{};
  std::array<std::array<int64_t, kCellCount>, kChannelCount> cell_counts{};
};

struct lifted_readout_t {
  torch::Tensor values{};     // [B,3,768], input dtype/device
  torch::Tensor valid_mask{}; // [B,3], bool, input device
};

// Reuse the existing serving adapter contract exactly: values [B,C,D] plus a
// bool [B,C] validity mask.  SRR-1 adds no parallel production-facing type.
using readout_output_t = mtf_serving_pool_output_t;

namespace detail {

struct metadata_record_t {
  int64_t source_index{0};
  int64_t domain{0};
  int64_t scale{0};
  int64_t start{0};
  int64_t width{0};
};

[[nodiscard]] inline torch::Tensor
metadata_vector_cpu(const torch::Tensor &value, const char *name) {
  TORCH_CHECK(value.defined(), "[SRR-1] missing metadata field ", name);
  TORCH_CHECK(value.dim() == 1 && value.numel() == kTokenCount,
              "[SRR-1] metadata field ", name, " must be [72]");
  TORCH_CHECK(value.scalar_type() == torch::kInt64,
              "[SRR-1] metadata field ", name, " must be int64");
  return value.to(torch::kCPU, torch::kInt64).contiguous();
}

[[nodiscard]] inline int64_t compact_cell_id(int64_t domain, int64_t scale,
                                              int64_t rank,
                                              int64_t group_size) {
  // Equal-width rank bins use token centres.  The two singleton scales occupy
  // one compact cell each; their mathematical middle-bin identity is retained
  // by the frozen compact offsets.
  const int64_t bin = std::min<int64_t>(
      2, (3 * (2 * rank + 1)) / (2 * group_size));
  const int64_t within_domain =
      scale < 2 ? kScaleCellOffsets[static_cast<std::size_t>(scale)] + bin
                : kScaleCellOffsets[static_cast<std::size_t>(scale)];
  return domain * 8 + within_domain;
}

inline void validate_config(
    const mtf_jepa_mae_vicreg_config_t &config) {
  TORCH_CHECK(config.channel_count == kChannelCount,
              "[SRR-1] shadow requires exactly 3 channels");
  TORCH_CHECK(config.history_length == kHistoryLength &&
                  config.input_width == kInputWidth &&
                  config.d_model == kLatentDim,
              "[SRR-1] shadow requires the frozen H=30,F=9,d_model=32 "
              "configuration");
  TORCH_CHECK(config.latent_dim == kLatentDim,
              "[SRR-1] shadow requires latent_dim=32");
  TORCH_CHECK(config.use_frequency_tokens,
              "[SRR-1] shadow requires both time and frequency domains");
  TORCH_CHECK(config.time_scales ==
                  std::vector<int64_t>(kTimeScales.begin(), kTimeScales.end()) &&
                  config.scale_strides == std::vector<int64_t>(
                                              kScaleStrides.begin(),
                                              kScaleStrides.end()),
              "[SRR-1] shadow requires scales 8,16,32,64 and strides "
              "4,8,16,32");
  TORCH_CHECK(config.serving_pool_policy ==
                  mtf_serving_pool_policy_t::all_tokens,
              "[SRR-1] shadow requires the frozen all_tokens baseline "
              "configuration");
}

inline void validate_encoded(
    const mtf_jepa_mae_vicreg_encode_output_t &encoded,
    const mtf_jepa_mae_vicreg_config_t &config) {
  validate_config(config);
  TORCH_CHECK(encoded.embeddings.defined() &&
                  encoded.embeddings.dim() == 3 &&
                  encoded.embeddings.size(1) == kTokenCount &&
                  encoded.embeddings.size(2) == kLatentDim,
              "[SRR-1] embeddings must be [B,72,32]");
  TORCH_CHECK(encoded.embeddings.is_floating_point(),
              "[SRR-1] embeddings must be floating point");
  TORCH_CHECK(torch::isfinite(encoded.embeddings).all().item<bool>(),
              "[SRR-1] embeddings must be finite");
  TORCH_CHECK(encoded.token_mask.defined() && encoded.token_mask.dim() == 2 &&
                  encoded.token_mask.size(0) == encoded.embeddings.size(0) &&
                  encoded.token_mask.size(1) == kTokenCount &&
                  encoded.token_mask.scalar_type() == torch::kBool,
              "[SRR-1] token_mask must be bool [B,72]");
  TORCH_CHECK(encoded.token_mask.device() == encoded.embeddings.device(),
              "[SRR-1] token_mask and embeddings must share a device");
  TORCH_CHECK(encoded.sample_valid_mask.defined() &&
                  encoded.sample_valid_mask.dim() == 1 &&
                  encoded.sample_valid_mask.size(0) ==
                      encoded.embeddings.size(0) &&
                  encoded.sample_valid_mask.scalar_type() == torch::kBool &&
                  encoded.sample_valid_mask.device() ==
                      encoded.embeddings.device(),
              "[SRR-1] sample_valid_mask must be bool [B] on input device");
  TORCH_CHECK(encoded.channel_valid_mask.defined() &&
                  encoded.channel_valid_mask.dim() == 2 &&
                  encoded.channel_valid_mask.size(0) ==
                      encoded.embeddings.size(0) &&
                  encoded.channel_valid_mask.size(1) == kChannelCount &&
                  encoded.channel_valid_mask.scalar_type() == torch::kBool &&
                  encoded.channel_valid_mask.device() ==
                      encoded.embeddings.device(),
              "[SRR-1] channel_valid_mask must be bool [B,3] on input "
              "device");
}

} // namespace detail

[[nodiscard]] inline readout_plan_t
build_plan(const mtf_token_metadata_t &metadata) {
  const auto channels =
      detail::metadata_vector_cpu(metadata.channel_id, "channel_id");
  const auto domains =
      detail::metadata_vector_cpu(metadata.domain_id, "domain_id");
  const auto scales =
      detail::metadata_vector_cpu(metadata.scale_id, "scale_id");
  const auto starts =
      detail::metadata_vector_cpu(metadata.start_index, "start_index");
  const auto widths = detail::metadata_vector_cpu(metadata.width, "width");
  const auto channel = channels.accessor<int64_t, 1>();
  const auto domain = domains.accessor<int64_t, 1>();
  const auto scale = scales.accessor<int64_t, 1>();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();

  std::array<std::vector<detail::metadata_record_t>, kChannelCount> records{};
  for (int64_t token = 0; token < kTokenCount; ++token) {
    TORCH_CHECK(channel[token] >= 0 && channel[token] < kChannelCount,
                "[SRR-1] channel_id is outside [0,2]");
    TORCH_CHECK(domain[token] >= 0 && domain[token] < kDomainCount,
                "[SRR-1] domain_id is outside [0,1]");
    TORCH_CHECK(scale[token] >= 0 && scale[token] < kScaleCount,
                "[SRR-1] scale_id is outside [0,3]");
    TORCH_CHECK(start[token] >= 0 && width[token] > 0,
                "[SRR-1] token start/width metadata is invalid");
    records[static_cast<std::size_t>(channel[token])].push_back(
        {.source_index = token,
         .domain = domain[token],
         .scale = scale[token],
         .start = start[token],
         .width = width[token]});
  }

  readout_plan_t plan{};
  std::array<std::array<int64_t, 4>, kTokensPerChannel> reference_layout{};
  for (int64_t channel_id = 0; channel_id < kChannelCount; ++channel_id) {
    auto &channel_records = records[static_cast<std::size_t>(channel_id)];
    TORCH_CHECK(channel_records.size() ==
                    static_cast<std::size_t>(kTokensPerChannel),
                "[SRR-1] each channel must contain exactly 24 tokens");
    std::sort(channel_records.begin(), channel_records.end(),
              [](const auto &left, const auto &right) {
                return std::tuple{left.domain, left.scale, left.start,
                                  left.width, left.source_index} <
                       std::tuple{right.domain, right.scale, right.start,
                                  right.width, right.source_index};
              });

    std::array<std::array<int64_t, kScaleCount>, kDomainCount> counts{};
    for (std::size_t position = 0; position < channel_records.size();
         ++position) {
      const auto &record = channel_records[position];
      if (position > 0) {
        const auto &previous = channel_records[position - 1];
        const bool unique_key =
            std::tuple{record.domain, record.scale, record.start, record.width} !=
            std::tuple{previous.domain, previous.scale, previous.start,
                       previous.width};
        TORCH_CHECK(unique_key,
            "[SRR-1] duplicate metadata makes rank bins ambiguous");
      }
      const std::array<int64_t, 4> layout_key{
          record.domain, record.scale, record.start, record.width};
      TORCH_CHECK(
          record.start == kFrozenStarts[position] &&
              record.width == kFrozenWidths[position],
          "[SRR-1] start/width layout does not match the frozen H=30 plan");
      if (channel_id == 0) {
        reference_layout[position] = layout_key;
      } else {
        TORCH_CHECK(layout_key == reference_layout[position],
                    "[SRR-1] all channels must share one metadata layout");
      }
      ++counts[static_cast<std::size_t>(record.domain)]
              [static_cast<std::size_t>(record.scale)];
    }
    for (int64_t domain_id = 0; domain_id < kDomainCount; ++domain_id) {
      for (int64_t scale_id = 0; scale_id < kScaleCount; ++scale_id) {
        TORCH_CHECK(
            counts[static_cast<std::size_t>(domain_id)]
                  [static_cast<std::size_t>(scale_id)] ==
                kTokensPerScale[static_cast<std::size_t>(scale_id)],
            "[SRR-1] each domain must have per-scale counts 7,3,1,1");
      }
    }

    std::array<int64_t, kCellCount> cell_counts{};
    for (int64_t position = 0; position < kTokensPerChannel; ++position) {
      const auto &record =
          channel_records[static_cast<std::size_t>(position)];
      int64_t group_begin = position;
      while (group_begin > 0) {
        const auto &candidate =
            channel_records[static_cast<std::size_t>(group_begin - 1)];
        if (candidate.domain != record.domain ||
            candidate.scale != record.scale) {
          break;
        }
        --group_begin;
      }
      const int64_t rank = position - group_begin;
      const int64_t group_size =
          counts[static_cast<std::size_t>(record.domain)]
                [static_cast<std::size_t>(record.scale)];
      const int64_t cell = detail::compact_cell_id(
          record.domain, record.scale, rank, group_size);
      TORCH_CHECK(cell == kFrozenCellIds[static_cast<std::size_t>(position)],
                  "[SRR-1] metadata-derived cells do not match PSM-1 CDSB");
      plan.ordered_token_indices[static_cast<std::size_t>(channel_id)]
                                [static_cast<std::size_t>(position)] =
          record.source_index;
      plan.ordered_cell_ids[static_cast<std::size_t>(channel_id)]
                           [static_cast<std::size_t>(position)] = cell;
      ++cell_counts[static_cast<std::size_t>(cell)];
    }
    TORCH_CHECK(cell_counts == kFrozenCellCounts,
                "[SRR-1] compact CDSB cells have unexpected cardinalities");
    plan.cell_counts[static_cast<std::size_t>(channel_id)] = cell_counts;
  }
  return plan;
}

[[nodiscard]] inline lifted_readout_t
lift(const mtf_jepa_mae_vicreg_encode_output_t &encoded,
     const mtf_jepa_mae_vicreg_config_t &config) {
  detail::validate_encoded(encoded, config);
  const auto plan = build_plan(encoded.metadata);
  const auto device = encoded.embeddings.device();
  std::vector<torch::Tensor> grouped_channels;
  std::vector<torch::Tensor> grouped_masks;
  grouped_channels.reserve(kChannelCount);
  grouped_masks.reserve(kChannelCount);
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    const auto &source =
        plan.ordered_token_indices[static_cast<std::size_t>(channel)];
    const std::vector<int64_t> indices(source.begin(), source.end());
    const auto index = torch::tensor(
        indices, torch::TensorOptions().dtype(torch::kInt64).device(device));
    grouped_channels.push_back(encoded.embeddings.index_select(1, index));
    grouped_masks.push_back(encoded.token_mask.index_select(1, index));
  }
  const auto grouped = torch::stack(grouped_channels, 1); // [B,3,24,32]
  const auto grouped_mask = torch::stack(grouped_masks, 1); // [B,3,24]

  std::vector<torch::Tensor> cell_means;
  cell_means.reserve(kCellCount);
  for (int64_t cell = 0; cell < kCellCount; ++cell) {
    std::vector<int64_t> positions;
    for (int64_t position = 0; position < kTokensPerChannel; ++position) {
      if (kFrozenCellIds[static_cast<std::size_t>(position)] == cell) {
        positions.push_back(position);
      }
    }
    const auto index = torch::tensor(
        positions, torch::TensorOptions().dtype(torch::kInt64).device(device));
    const auto selected = grouped.index_select(2, index);
    const auto selected_mask = grouped_mask.index_select(2, index);
    const auto all_valid = selected_mask.all(2);
    const auto raw_mean = selected.mean(2);
    const auto weight = selected_mask.to(selected.scalar_type()).unsqueeze(-1);
    const auto masked_mean = (selected * weight).sum(2) /
                             weight.sum(2).clamp_min(1.0);
    cell_means.push_back(torch::where(all_valid.unsqueeze(-1), raw_mean,
                                      masked_mean));
  }

  // PSM-1 established CDSB only for fully observed 24-token channel blocks.
  // SRR-1 therefore fails closed on every partial block.  Generalizing a
  // partial-mask pooling policy would be a separate scientific experiment.
  auto valid_mask = grouped_mask.all(2);
  valid_mask = valid_mask.logical_and(encoded.sample_valid_mask.unsqueeze(1));
  valid_mask = valid_mask.logical_and(encoded.channel_valid_mask);
  std::vector<torch::Tensor> lifted_positions;
  lifted_positions.reserve(kTokensPerChannel);
  for (const int64_t cell : kFrozenCellIds) {
    lifted_positions.push_back(cell_means[static_cast<std::size_t>(cell)]);
  }
  auto values = torch::stack(lifted_positions, 2)
                    .reshape({encoded.embeddings.size(0), kChannelCount,
                              kProjectionInputWidth})
                    .contiguous();
  values = torch::where(valid_mask.unsqueeze(-1), values,
                        torch::zeros_like(values));
  return {.values = std::move(values),
          .valid_mask = std::move(valid_mask)};
}

[[nodiscard]] inline readout_output_t
readout(const mtf_jepa_mae_vicreg_encode_output_t &encoded,
        const torch::Tensor &projection,
        const mtf_jepa_mae_vicreg_config_t &config) {
  TORCH_CHECK(projection.defined() && projection.dim() == 2 &&
                  projection.size(0) == kProjectionInputWidth &&
                  projection.size(1) == kProjectionOutputWidth,
              "[SRR-1] fixed projection must be [768,32]");
  TORCH_CHECK(projection.is_floating_point(),
              "[SRR-1] fixed projection must be floating point");
  TORCH_CHECK(projection.scalar_type() == torch::kFloat64 &&
                  projection.device().is_cpu() && projection.is_contiguous(),
              "[SRR-1] fixed projection must be contiguous CPU float64");
  TORCH_CHECK(!projection.requires_grad(),
              "[SRR-1] fixed projection must not require gradients");
  TORCH_CHECK(torch::isfinite(projection).all().item<bool>(),
              "[SRR-1] fixed projection must be finite");
  auto lifted = lift(encoded, config);
  const auto fixed_projection =
      projection
          .to(encoded.embeddings.device(), encoded.embeddings.scalar_type())
          .contiguous();
  auto values = lifted.values.matmul(fixed_projection).contiguous();
  values = torch::where(lifted.valid_mask.unsqueeze(-1), values,
                        torch::zeros_like(values));
  return {.values = std::move(values),
          .valid_mask = std::move(lifted.valid_mask)};
}

// Audit entry: the public encoder result is detached and translated once to
// CPU float64 before the exact same metadata-driven computation.  This mirrors
// the sealed PSM capture boundary and makes numerical identity easy to audit.
[[nodiscard]] inline readout_output_t
readout_cpu64(const mtf_jepa_mae_vicreg_encode_output_t &encoded,
              const torch::Tensor &projection,
              const mtf_jepa_mae_vicreg_config_t &config) {
  mtf_jepa_mae_vicreg_encode_output_t audit{};
  audit.embeddings =
      encoded.embeddings.detach().to(torch::kCPU, torch::kFloat64).contiguous();
  audit.token_mask = encoded.token_mask.detach().to(torch::kCPU).contiguous();
  audit.sample_valid_mask =
      encoded.sample_valid_mask.detach().to(torch::kCPU).contiguous();
  audit.channel_valid_mask =
      encoded.channel_valid_mask.detach().to(torch::kCPU).contiguous();
  audit.metadata = encoded.metadata;
  const auto fixed_projection =
      projection.detach().to(torch::kCPU, torch::kFloat64).contiguous();
  auto audit_config = config;
  audit_config.dtype = torch::kFloat64;
  audit_config.device = torch::Device(torch::kCPU);
  return readout(audit, fixed_projection, audit_config);
}

} // namespace cuwacunu::wikimyei::representation::encoding::
  // mtf_jepa_mae_vicreg::structured_readout_shadow
