// ./include/wikimyei/evaluation/embedding_sequence_analytics/embedding_sequence_analytics.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <torch/torch.h>

#include "iitepi/contract_space_t.h"
#include "piaabo/torch_compat/data_analytics.h"

namespace cuwacunu::wikimyei::evaluation {

class VicregEmbeddingSequenceEvaluator final {
 public:
  using Options = cuwacunu::piaabo::torch_compat::data_analytics_options_t;
  using SequenceReport =
      cuwacunu::piaabo::torch_compat::sequence_analytics_report_t;
  using SequenceSymbolicReport =
      cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_report_t;

  struct Snapshot {
    Options options{};
    SequenceReport report{};
    SequenceSymbolicReport symbolic_report{};
  };

  explicit VicregEmbeddingSequenceEvaluator(std::string contract_hash,
                                            std::int64_t stream_count)
      : contract_hash_(std::move(contract_hash)),
        stream_count_(stream_count) {
    load_config_();
    reset();
  }

  [[nodiscard]] static constexpr std::string_view evaluator_name() noexcept {
    return "wikimyei.evaluation.embedding_sequence_analytics";
  }

  void reset() {
    analytics_accumulator_ =
        cuwacunu::piaabo::torch_compat::sequence_analytics_accumulator_t(
            options_);
    symbolic_accumulator_ = cuwacunu::piaabo::torch_compat::
        sequence_symbolic_analytics_accumulator_t(
            build_stream_descriptors_(stream_count_));
    observed_ = false;
  }

  void ingest_encoding(const torch::Tensor& encoding,
                       const torch::Tensor& source_mask) {
    torch::Tensor sequence_features;
    torch::Tensor sequence_mask;
    if (!reshape_embedding_sequence_for_analytics_(
            encoding, source_mask, &sequence_features, &sequence_mask)) {
      return;
    }

    const bool ingested_numeric =
        analytics_accumulator_.ingest(sequence_features, sequence_mask);
    const bool ingested_symbolic =
        symbolic_accumulator_.ingest(sequence_features, sequence_mask);
    observed_ = observed_ || ingested_numeric || ingested_symbolic;
  }

  [[nodiscard]] std::optional<Snapshot> snapshot() const {
    if (!observed_) return std::nullopt;
    Snapshot out{};
    out.options = options_;
    out.report = analytics_accumulator_.summarize();
    out.symbolic_report = symbolic_accumulator_.summarize();
    return out;
  }

  [[nodiscard]] const Options& options() const noexcept { return options_; }

 private:
  static constexpr const char* kContractSection =
      "WIKIMYEI_EVALUATION_EMBEDDING_SEQUENCE_ANALYTICS";

  [[nodiscard]] static std::vector<
      cuwacunu::piaabo::torch_compat::sequence_symbolic_stream_descriptor_t>
  build_stream_descriptors_(std::int64_t stream_count) {
    using descriptor_t = cuwacunu::piaabo::torch_compat::
        sequence_symbolic_stream_descriptor_t;
    std::vector<descriptor_t> out;
    if (stream_count <= 0) return out;
    out.reserve(static_cast<std::size_t>(stream_count));

    std::int64_t width = 1;
    for (std::int64_t value = std::max<std::int64_t>(0, stream_count - 1);
         value >= 10;
         value /= 10) {
      ++width;
    }

    for (std::int64_t i = 0; i < stream_count; ++i) {
      std::ostringstream label;
      label << "latent_dim_" << std::setw(static_cast<int>(width))
            << std::setfill('0') << i;
      descriptor_t descriptor{};
      descriptor.label = label.str();
      descriptor.stream_family = "embedding_dimension";
      descriptor.anchor_feature = "value";
      descriptor.feature_names = "value";
      descriptor.anchor_feature_index = 0;
      out.push_back(std::move(descriptor));
    }
    return out;
  }

  [[nodiscard]] static bool build_sequence_mask_(const torch::Tensor& source_mask,
                                                 std::int64_t batch_size,
                                                 std::int64_t timesteps,
                                                 std::int64_t stream_count,
                                                 torch::Tensor* out_mask) {
    if (!out_mask || batch_size <= 0 || timesteps <= 0 || stream_count <= 0) {
      return false;
    }
    out_mask->reset();

    torch::Tensor collapsed;
    if (source_mask.defined() && source_mask.numel() > 0) {
      torch::Tensor m = source_mask.to(torch::kFloat64);
      if (m.dim() == 1) {
        if (batch_size != 1 || m.size(0) != timesteps) return false;
        collapsed = m.view({1, 1, timesteps});
      } else if (m.dim() == 2) {
        if (batch_size != 1 || m.size(1) != timesteps) return false;
        collapsed = m.amax({0}, /*keepdim=*/false).view({1, 1, timesteps});
      } else if (m.dim() == 3) {
        if (m.size(0) != batch_size || m.size(2) != timesteps) return false;
        collapsed = m.amax({1}, /*keepdim=*/true);
      } else {
        return false;
      }
      collapsed = collapsed.clamp_min(0.0);
    } else {
      collapsed = torch::ones(
          {batch_size, 1, timesteps},
          torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
    }

    *out_mask = collapsed.expand({batch_size, stream_count, timesteps});
    return true;
  }

  [[nodiscard]] static bool reshape_embedding_sequence_for_analytics_(
      const torch::Tensor& encoding,
      const torch::Tensor& source_mask,
      torch::Tensor* out_features,
      torch::Tensor* out_mask) {
    if (!out_features || !out_mask) return false;
    out_features->reset();
    out_mask->reset();
    if (!encoding.defined() || encoding.numel() == 0) return false;

    torch::Tensor seq = encoding;
    if (seq.dim() == 2) {
      seq = seq.unsqueeze(0);
    } else if (seq.dim() != 3) {
      return false;
    }
    if (seq.size(0) <= 0 || seq.size(1) <= 0 || seq.size(2) <= 0) return false;

    const std::int64_t batch_size = seq.size(0);
    const std::int64_t timesteps = seq.size(1);
    const std::int64_t stream_count = seq.size(2);
    seq = seq.transpose(1, 2).unsqueeze(-1);

    torch::Tensor mask;
    if (!build_sequence_mask_(
            source_mask, batch_size, timesteps, stream_count, &mask)) {
      return false;
    }

    *out_features = std::move(seq);
    *out_mask = std::move(mask);
    return true;
  }

  void load_config_() {
    const auto contract =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash_);
    options_.max_samples = static_cast<std::int64_t>(contract->get<int>(
        kContractSection, "max_samples", std::optional<int>{4096}));
    options_.max_features = static_cast<std::int64_t>(contract->get<int>(
        kContractSection, "max_features", std::optional<int>{2048}));
    options_.mask_epsilon = contract->get<double>(
        kContractSection, "mask_epsilon", std::optional<double>{1e-12});
    options_.standardize_epsilon = contract->get<double>(
        kContractSection,
        "standardize_epsilon",
        std::optional<double>{1e-8});

    if (options_.max_samples <= 0) options_.max_samples = 4096;
    if (options_.max_features <= 0) options_.max_features = 2048;
    if (options_.mask_epsilon < 0.0) options_.mask_epsilon = 1e-12;
    if (options_.standardize_epsilon <= 0.0) {
      options_.standardize_epsilon = 1e-8;
    }
  }

  std::string contract_hash_{};
  std::int64_t stream_count_{0};
  bool observed_{false};
  Options options_{};
  cuwacunu::piaabo::torch_compat::sequence_analytics_accumulator_t
      analytics_accumulator_{};
  cuwacunu::piaabo::torch_compat::sequence_symbolic_analytics_accumulator_t
      symbolic_accumulator_{};
};

}  // namespace cuwacunu::wikimyei::evaluation
