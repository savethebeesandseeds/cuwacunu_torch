/* mixture_density_network_head.h */
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace inference {
namespace expected_value {
namespace mdn {

struct ChannelAdapterImpl : torch::nn::Module {
  int64_t H{0};
  int64_t rank{0};
  torch::nn::LayerNorm norm{nullptr};
  torch::nn::Linear down{nullptr};
  torch::nn::Linear up{nullptr};

  explicit ChannelAdapterImpl(const ChannelAdapterOptions &opt)
      : H(opt.feature_dim), rank(opt.adapter_rank) {
    TORCH_CHECK(H > 0 && rank > 0,
                "[ChannelAdapter] invalid feature/adaptor dimensions");
    norm = register_module(
        "norm", torch::nn::LayerNorm(
                    torch::nn::LayerNormOptions(std::vector<int64_t>{H})));
    down = register_module("down", torch::nn::Linear(H, rank));
    up = register_module("up", torch::nn::Linear(rank, H));
    {
      torch::NoGradGuard ng;
      up->weight.zero_();
      if (up->bias.defined()) {
        up->bias.zero_();
      }
    }
  }

  // h: [...,H] -> [...,H].  The adapter is residual and initialized as an
  // identity-preserving no-op; channel specialization grows only when trained.
  torch::Tensor forward(const torch::Tensor &h) {
    TORCH_CHECK(h.defined() && h.size(-1) == H,
                "[ChannelAdapter] h must end with H");
    auto delta = up->forward(torch::silu(down->forward(norm->forward(h))));
    return h + delta;
  }
};
TORCH_MODULE(ChannelAdapter);

struct ChannelAdapterStackImpl : torch::nn::Module {
  int64_t C{0};
  int64_t H{0};
  int64_t rank{0};
  std::vector<ChannelAdapter> adapters{};

  ChannelAdapterStackImpl(int64_t C_, int64_t H_, int64_t rank_)
      : C(C_), H(H_), rank(rank_) {
    TORCH_CHECK(C > 0 && H > 0 && rank > 0,
                "[ChannelAdapterStack] invalid dimensions");
    adapters.reserve(static_cast<std::size_t>(C));
    for (int64_t c = 0; c < C; ++c) {
      auto adapter = ChannelAdapter(
          ChannelAdapterOptions{.feature_dim = H, .adapter_rank = rank});
      adapters.push_back(
          register_module("channel_adapter_" + std::to_string(c), adapter));
    }
  }

  // h: [B,N,C,H] -> [B,N,C,H]
  torch::Tensor forward(const torch::Tensor &h) {
    TORCH_CHECK(h.defined() && h.dim() == 4,
                "[ChannelAdapterStack] h must be [B,N,C,H]");
    TORCH_CHECK(h.size(2) == C && h.size(3) == H,
                "[ChannelAdapterStack] h shape mismatch");
    using torch::indexing::Slice;
    std::vector<torch::Tensor> adapted;
    adapted.reserve(static_cast<std::size_t>(C));
    for (int64_t c = 0; c < C; ++c) {
      auto hc = h.index({Slice(), Slice(), c, Slice()});
      adapted.push_back(
          adapters[static_cast<std::size_t>(c)]->forward(hc).unsqueeze(2));
    }
    return torch::cat(adapted, /*dim=*/2);
  }
};
TORCH_MODULE(ChannelAdapterStack);

struct FeatureConditionedMdnHeadImpl : torch::nn::Module {
  int64_t H{0};
  int64_t Df{0};
  int64_t K{0};
  int64_t Ef{0};
  int64_t source_feature_vocab_size{0};
  double sigma_floor{1e-3};
  torch::nn::Embedding feature_embedding{nullptr};
  torch::Tensor target_coord_ids{};
  torch::nn::LayerNorm input_norm{nullptr};
  torch::nn::Linear hidden{nullptr};
  torch::nn::Linear projection{nullptr};

  explicit FeatureConditionedMdnHeadImpl(
      const FeatureConditionedMdnHeadOptions &opt)
      : H(opt.feature_dim), Df(opt.target_feature_dim), K(opt.mixture_count),
        Ef(opt.feature_embedding_dim), sigma_floor(opt.sigma_floor) {
    TORCH_CHECK(H > 0 && Df > 0 && K > 0 && Ef > 0,
                "[FeatureConditionedMdnHead] invalid dimensions");
    TORCH_CHECK(sigma_floor >= 0.0,
                "[FeatureConditionedMdnHead] sigma_floor must be "
                "nonnegative");
    std::vector<int64_t> coords = opt.target_coords;
    if (coords.empty()) {
      coords.reserve(static_cast<std::size_t>(Df));
      for (int64_t i = 0; i < Df; ++i) {
        coords.push_back(i);
      }
    }
    TORCH_CHECK(static_cast<int64_t>(coords.size()) == Df,
                "[FeatureConditionedMdnHead] target_coords size must match "
                "target_feature_dim");
    int64_t max_coord = -1;
    for (const auto coord : coords) {
      TORCH_CHECK(coord >= 0,
                  "[FeatureConditionedMdnHead] target coord must be "
                  "nonnegative");
      max_coord = std::max(max_coord, coord);
    }
    source_feature_vocab_size = opt.source_feature_vocab_size > 0
                                    ? opt.source_feature_vocab_size
                                    : (max_coord + 1);
    TORCH_CHECK(source_feature_vocab_size > max_coord,
                "[FeatureConditionedMdnHead] source feature vocabulary is too "
                "small for target_coords");
    feature_embedding = register_module(
        "feature_embedding", torch::nn::Embedding(torch::nn::EmbeddingOptions(
                                 source_feature_vocab_size, Ef)));
    target_coord_ids = register_buffer(
        "target_coord_ids",
        torch::tensor(coords, torch::TensorOptions().dtype(torch::kInt64)));
    input_norm = register_module(
        "input_norm", torch::nn::LayerNorm(torch::nn::LayerNormOptions(
                          std::vector<int64_t>{H + Ef})));
    hidden = register_module("hidden", torch::nn::Linear(H + Ef, H));
    projection = register_module("projection", torch::nn::Linear(H, 3 * K));
    {
      torch::NoGradGuard ng;
      if (projection->bias.defined()) {
        projection->bias.zero_();
        const double init_sigma = std::max(0.1 - sigma_floor, 1e-6);
        const double sigma_bias = softplus_inv(init_sigma);
        projection->bias.slice(/*dim=*/0, 2 * K, 3 * K).fill_(sigma_bias);
      }
    }
  }

  // h: [B,N,C,H] -> log_pi/mu/sigma [B,N,C,Df,K].
  MdnOut forward(const torch::Tensor &h) {
    TORCH_CHECK(h.defined() && h.dim() == 4,
                "[FeatureConditionedMdnHead] h must be [B,N,C,H]");
    TORCH_CHECK(h.size(3) == H, "[FeatureConditionedMdnHead] h shape mismatch");
    const auto B = h.size(0);
    const auto N = h.size(1);
    const auto C = h.size(2);
    auto ids = target_coord_ids.to(
        torch::TensorOptions().dtype(torch::kInt64).device(h.device()));
    auto ef = feature_embedding->forward(ids).to(h.dtype()); // [Df,Ef]
    auto h_expanded = h.unsqueeze(3).expand({B, N, C, Df, H});
    auto ef_expanded = ef.view({1, 1, 1, Df, Ef}).expand({B, N, C, Df, Ef});
    auto x = torch::cat({h_expanded, ef_expanded}, /*dim=*/-1)
                 .contiguous()
                 .view({B * N * C * Df, H + Ef});
    auto z = torch::silu(hidden->forward(input_norm->forward(x)));
    auto raw = projection->forward(z).view({B, N, C, Df, 3, K});
    auto raw_pi = raw.select(/*dim=*/4, /*index=*/0);
    auto raw_mu = raw.select(/*dim=*/4, /*index=*/1);
    auto raw_sigma = raw.select(/*dim=*/4, /*index=*/2);
    return {torch::log_softmax(raw_pi, /*dim=*/-1), raw_mu,
            positive_sigma_from_raw(raw_sigma, sigma_floor), torch::Tensor{}};
  }
};
TORCH_MODULE(FeatureConditionedMdnHead);

struct DirectEdgeReturnHeadImpl : torch::nn::Module {
  int64_t H{0};
  int64_t quote_node_index{0};
  std::string identity_mode{"shared"};
  int64_t base_edge_count{0};
  int64_t identity_embedding_dim{0};
  int64_t adapter_hidden_dim{0};
  bool uses_edge_embedding{false};
  bool uses_per_edge_projection{false};
  torch::nn::LayerNorm adapter_norm{nullptr};
  torch::nn::Linear adapter_down{nullptr};
  torch::nn::Linear adapter_up{nullptr};
  torch::nn::Embedding edge_embedding{nullptr};
  torch::nn::LayerNorm input_norm{nullptr};
  torch::nn::Linear hidden{nullptr};
  torch::nn::Linear projection{nullptr};
  std::vector<torch::nn::Linear> edge_projections{};

  explicit DirectEdgeReturnHeadImpl(const DirectEdgeReturnHeadOptions &opt)
      : H(opt.feature_dim), quote_node_index(opt.quote_node_index),
        identity_mode(opt.identity_mode), base_edge_count(opt.base_edge_count),
        identity_embedding_dim(opt.identity_embedding_dim),
        adapter_hidden_dim(opt.adapter_hidden_dim) {
    TORCH_CHECK(H > 0, "[DirectEdgeReturnHead] hidden width must be positive");
    TORCH_CHECK(quote_node_index == 0,
                "[DirectEdgeReturnHead] v1 expects quote node index 0");
    TORCH_CHECK(
        identity_mode == "shared" || identity_mode == "edge_embedding" ||
            identity_mode == "per_edge" ||
            identity_mode == "edge_embedding_per_edge",
        "[DirectEdgeReturnHead] invalid identity mode: ", identity_mode);
    uses_edge_embedding = identity_mode == "edge_embedding" ||
                          identity_mode == "edge_embedding_per_edge";
    uses_per_edge_projection = identity_mode == "per_edge" ||
                               identity_mode == "edge_embedding_per_edge";
    TORCH_CHECK(base_edge_count >= 0,
                "[DirectEdgeReturnHead] base_edge_count must be nonnegative");
    if (identity_mode != "shared") {
      TORCH_CHECK(base_edge_count > 0,
                  "[DirectEdgeReturnHead] identity/per-edge modes require "
                  "base_edge_count > 0");
    }
    TORCH_CHECK(identity_embedding_dim >= 0,
                "[DirectEdgeReturnHead] identity_embedding_dim must be "
                "nonnegative");
    if (uses_edge_embedding) {
      TORCH_CHECK(identity_embedding_dim > 0,
                  "[DirectEdgeReturnHead] edge embedding modes require "
                  "identity_embedding_dim > 0");
      edge_embedding = register_module(
          "edge_embedding", torch::nn::Embedding(torch::nn::EmbeddingOptions(
                                base_edge_count, identity_embedding_dim)));
    } else {
      TORCH_CHECK(identity_embedding_dim == 0,
                  "[DirectEdgeReturnHead] identity_embedding_dim must be 0 "
                  "unless edge embedding mode is active");
    }
    TORCH_CHECK(adapter_hidden_dim >= 0,
                "[DirectEdgeReturnHead] adapter_hidden_dim must be "
                "nonnegative");
    if (adapter_hidden_dim > 0) {
      adapter_norm = register_module(
          "adapter_norm", torch::nn::LayerNorm(torch::nn::LayerNormOptions(
                              std::vector<int64_t>{H})));
      adapter_down = register_module("adapter_down",
                                     torch::nn::Linear(H, adapter_hidden_dim));
      adapter_up = register_module("adapter_up",
                                   torch::nn::Linear(adapter_hidden_dim, H));
      torch::NoGradGuard ng;
      adapter_up->weight.zero_();
      if (adapter_up->bias.defined()) {
        adapter_up->bias.zero_();
      }
    }
    const int64_t input_dim =
        3 * H + (uses_edge_embedding ? identity_embedding_dim : 0);
    input_norm = register_module(
        "input_norm", torch::nn::LayerNorm(torch::nn::LayerNormOptions(
                          std::vector<int64_t>{input_dim})));
    hidden = register_module("hidden", torch::nn::Linear(input_dim, H));
    if (uses_per_edge_projection) {
      edge_projections.reserve(static_cast<std::size_t>(base_edge_count));
      for (int64_t edge = 0; edge < base_edge_count; ++edge) {
        auto edge_projection = torch::nn::Linear(H, 1);
        edge_projections.push_back(register_module(
            "projection_edge_" + std::to_string(edge), edge_projection));
      }
    } else {
      projection = register_module("projection", torch::nn::Linear(H, 1));
    }
    {
      torch::NoGradGuard ng;
      if (!projection.is_empty() && projection->bias.defined()) {
        projection->bias.zero_();
      }
      for (auto &edge_projection : edge_projections) {
        if (edge_projection->bias.defined()) {
          edge_projection->bias.zero_();
        }
      }
    }
  }

  explicit DirectEdgeReturnHeadImpl(int64_t H_, int64_t quote_node_index_ = 0)
      : DirectEdgeReturnHeadImpl(DirectEdgeReturnHeadOptions{
            .feature_dim = H_, .quote_node_index = quote_node_index_}) {}

  torch::Tensor adapted_context(const torch::Tensor &h) {
    if (adapter_hidden_dim <= 0) {
      return h;
    }
    auto flat = h.contiguous().view({-1, H});
    auto delta = adapter_up
                     ->forward(torch::silu(
                         adapter_down->forward(adapter_norm->forward(flat))))
                     .view_as(h);
    return h + delta;
  }

  // h: [B,N,C,H] -> direct head features [B,N-1,C,3H].
  torch::Tensor edge_features(const torch::Tensor &h) {
    TORCH_CHECK(h.defined() && h.dim() == 4,
                "[DirectEdgeReturnHead] h must be [B,N,C,H]");
    TORCH_CHECK(h.size(1) > 1,
                "[DirectEdgeReturnHead] at least one base node is required");
    TORCH_CHECK(h.size(3) == H, "[DirectEdgeReturnHead] h shape mismatch");
    const auto B = h.size(0);
    const auto N = h.size(1);
    const auto C = h.size(2);
    const auto base_count = N - 1;
    auto quote = h.select(1, quote_node_index)
                     .unsqueeze(1)
                     .expand({B, base_count, C, H});
    auto base = h.narrow(1, 1, base_count);
    return torch::cat({base, quote, base - quote}, /*dim=*/-1).contiguous();
  }

  // h: [B,N,C,H] -> direct readout input [B,N-1,C,3H(+Eid)].
  torch::Tensor readout_input_features(const torch::Tensor &h) {
    auto adapted = adapted_context(h);
    auto features = edge_features(adapted);
    if (!uses_edge_embedding) {
      return features;
    }
    const auto B = features.size(0);
    const auto E = features.size(1);
    const auto C = features.size(2);
    TORCH_CHECK(E <= base_edge_count,
                "[DirectEdgeReturnHead] observed base edge count exceeds "
                "configured base_edge_count");
    auto ids = torch::arange(
        E,
        torch::TensorOptions().dtype(torch::kInt64).device(features.device()));
    auto embedding = edge_embedding->forward(ids)
                         .to(features.dtype())
                         .view({1, E, 1, identity_embedding_dim})
                         .expand({B, E, C, identity_embedding_dim});
    return torch::cat({features, embedding}, /*dim=*/-1).contiguous();
  }

  // h: [B,N,C,H] -> direct base-minus-quote edge return [B,N-1,C].
  torch::Tensor forward(const torch::Tensor &h) {
    const auto B = h.size(0);
    const auto N = h.size(1);
    const auto C = h.size(2);
    const auto base_count = N - 1;
    TORCH_CHECK(!uses_per_edge_projection || base_count <= base_edge_count,
                "[DirectEdgeReturnHead] observed base edge count exceeds "
                "configured base_edge_count");
    auto x = readout_input_features(h).view({B * base_count * C, -1});
    auto z = torch::silu(hidden->forward(input_norm->forward(x)))
                 .view({B, base_count, C, H});
    if (!uses_per_edge_projection) {
      return projection->forward(z.view({B * base_count * C, H}))
          .view({B, base_count, C});
    }
    using torch::indexing::Slice;
    std::vector<torch::Tensor> edge_outputs;
    edge_outputs.reserve(static_cast<std::size_t>(base_count));
    for (int64_t edge = 0; edge < base_count; ++edge) {
      auto ze = z.index({Slice(), edge, Slice(), Slice()})
                    .contiguous()
                    .view({B * C, H});
      edge_outputs.push_back(
          edge_projections[static_cast<std::size_t>(edge)]->forward(ze).view(
              {B, 1, C}));
    }
    return torch::cat(edge_outputs, /*dim=*/1);
  }
};
TORCH_MODULE(DirectEdgeReturnHead);

} // namespace mdn
} // namespace expected_value
} // namespace inference
} // namespace wikimyei
} // namespace cuwacunu
