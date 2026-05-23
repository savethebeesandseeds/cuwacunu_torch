// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_backbone.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_head.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_loss.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn {

struct channel_mdn_input_t {
  torch::Tensor context{};      // [B_node,C,De]
  torch::Tensor context_mask{}; // [B_node,C], bool
  torch::Tensor future{};       // [B_node,C,Hf,Df]
  torch::Tensor future_mask{};  // [B_node,C,Hf], bool
  torch::Tensor anchor_index{}; // [B_node], int64
  torch::Tensor node_index{};   // [B_node], int64
  torch::Tensor anchor_keys{};  // [B]
  std::vector<int64_t> target_coords{};
  std::vector<std::string> node_ids{};
};

struct channel_mdn_plus_global_input_t {
  channel_mdn_input_t channel{};
  torch::Tensor global_context{}; // [B_node,Dg]
  torch::Tensor global_mask{};    // [B_node], bool
};

namespace channel_context_mdn_detail {

inline void validate_target_coords(const std::vector<int64_t> &coords,
                                   int64_t feature_width) {
  TORCH_CHECK(!coords.empty(),
              "[channel_context_mdn] target feature coords are empty");
  std::unordered_set<int64_t> seen;
  seen.reserve(coords.size());
  for (const int64_t coord : coords) {
    TORCH_CHECK(coord >= 0 && coord < feature_width,
                "[channel_context_mdn] target coord out of range: ", coord,
                " for feature width ", feature_width);
    TORCH_CHECK(seen.insert(coord).second,
                "[channel_context_mdn] duplicate target coord: ", coord);
  }
}

} // namespace channel_context_mdn_detail

[[nodiscard]] inline torch::Tensor
combine_channel_context_and_future_mask(const torch::Tensor &context_mask,
                                        const torch::Tensor &future_mask) {
  TORCH_CHECK(context_mask.defined() && context_mask.dim() == 2,
              "[channel_context_mdn] context_mask must be [B_node,C]");
  TORCH_CHECK(future_mask.defined() && future_mask.dim() == 3,
              "[channel_context_mdn] future_mask must be [B_node,C,Hf]");
  TORCH_CHECK(context_mask.size(0) == future_mask.size(0) &&
                  context_mask.size(1) == future_mask.size(1),
              "[channel_context_mdn] context/future mask shape mismatch");
  return future_mask.to(torch::kBool)
      .logical_and(context_mask.to(torch::kBool).unsqueeze(-1));
}

[[nodiscard]] inline torch::Tensor
combine_channel_plus_global_context_and_future_mask(
    const torch::Tensor &context_mask, const torch::Tensor &global_mask,
    const torch::Tensor &future_mask) {
  TORCH_CHECK(global_mask.defined() && global_mask.dim() == 1,
              "[channel_context_mdn] global_mask must be [B_node]");
  auto combined =
      combine_channel_context_and_future_mask(context_mask, future_mask);
  TORCH_CHECK(global_mask.size(0) == combined.size(0),
              "[channel_context_mdn] global/context mask shape mismatch");
  return combined.logical_and(
      global_mask.to(torch::kBool).unsqueeze(-1).unsqueeze(-1));
}

struct ChannelContextHeadsImpl : torch::nn::Module {
  int64_t C{0};
  int64_t Hf{0};
  int64_t Df{0};
  int64_t K{0};
  int64_t H{0};
  std::vector<cuwacunu::wikimyei::inference::expected_value::mdn::MdnHead>
      heads{};

  ChannelContextHeadsImpl(int64_t C_, int64_t Hf_, int64_t Df_, int64_t K_,
                          int64_t H_)
      : C(C_), Hf(Hf_), Df(Df_), K(K_), H(H_) {
    TORCH_CHECK(C > 0 && Hf > 0 && Df > 0 && K > 0 && H > 0,
                "[ChannelContextHeads] invalid dimensions");
    heads.reserve(static_cast<std::size_t>(C));
    for (int64_t c = 0; c < C; ++c) {
      cuwacunu::wikimyei::inference::expected_value::mdn::MdnHeadOptions opts{
          H, Df, K, Hf};
      auto head =
          cuwacunu::wikimyei::inference::expected_value::mdn::MdnHead(opts);
      heads.push_back(
          register_module("channel_head_" + std::to_string(c), head));
    }
  }

  cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut
  forward(const torch::Tensor &h) {
    TORCH_CHECK(h.defined() && h.dim() == 3,
                "[ChannelContextHeads] h must be [B_node,C,H]");
    TORCH_CHECK(h.size(1) == C && h.size(2) == H,
                "[ChannelContextHeads] h shape mismatch");
    using torch::indexing::Slice;
    std::vector<torch::Tensor> log_pi;
    std::vector<torch::Tensor> mu;
    std::vector<torch::Tensor> sigma;
    log_pi.reserve(static_cast<std::size_t>(C));
    mu.reserve(static_cast<std::size_t>(C));
    sigma.reserve(static_cast<std::size_t>(C));
    for (int64_t c = 0; c < C; ++c) {
      auto hc = h.index({Slice(), c, Slice()});
      auto out = heads[static_cast<std::size_t>(c)]->forward(hc);
      const auto B = h.size(0);
      TORCH_CHECK(out.log_pi.sizes() == torch::IntArrayRef({B, 1, Hf, K}),
                  "[ChannelContextHeads] per-channel log_pi shape mismatch");
      TORCH_CHECK(out.mu.sizes() == torch::IntArrayRef({B, 1, Hf, K, Df}),
                  "[ChannelContextHeads] per-channel mu shape mismatch");
      TORCH_CHECK(out.sigma.sizes() == torch::IntArrayRef({B, 1, Hf, K, Df}),
                  "[ChannelContextHeads] per-channel sigma shape mismatch");
      log_pi.push_back(out.log_pi);
      mu.push_back(out.mu);
      sigma.push_back(out.sigma);
    }
    return {torch::cat(log_pi, /*dim=*/1), torch::cat(mu, /*dim=*/1),
            torch::cat(sigma, /*dim=*/1)};
  }
};

TORCH_MODULE(ChannelContextHeads);

struct ChannelContextMdnImpl : torch::nn::Module {
  int64_t De{0};
  int64_t Df{0};
  int64_t C{0};
  int64_t Hf{0};
  int64_t K{0};
  int64_t H{0};
  int64_t depth{0};
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};

  cuwacunu::wikimyei::inference::expected_value::mdn::Backbone backbone{
      nullptr};
  ChannelContextHeads heads{nullptr};

  ChannelContextMdnImpl(int64_t De_, int64_t Df_, int64_t C_, int64_t Hf_,
                        int64_t K_, int64_t H_, int64_t depth_,
                        torch::Dtype dtype_ = torch::kFloat32,
                        torch::Device device_ = torch::kCPU)
      : De(De_), Df(Df_), C(C_), Hf(Hf_), K(K_), H(H_), depth(depth_),
        dtype(dtype_), device(device_) {
    TORCH_CHECK(De > 0 && Df > 0 && C > 0 && Hf > 0 && K > 0 && H > 0 &&
                    depth >= 0,
                "[ChannelContextMdn] invalid dimensions");
    cuwacunu::wikimyei::inference::expected_value::mdn::BackboneOptions bopts{
        De, H, depth};
    backbone = register_module(
        "backbone",
        cuwacunu::wikimyei::inference::expected_value::mdn::Backbone(bopts));
    heads = register_module("heads", ChannelContextHeads(C, Hf, Df, K, H));
    this->to(device, dtype);
  }

  cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut
  forward(const torch::Tensor &channel_context) {
    TORCH_CHECK(channel_context.defined() && channel_context.dim() == 3,
                "[ChannelContextMdn] context must be [B_node,C,De]");
    TORCH_CHECK(channel_context.size(1) == C && channel_context.size(2) == De,
                "[ChannelContextMdn] context shape mismatch");
    const auto rows = channel_context.size(0);
    auto x =
        channel_context.to(torch::TensorOptions().dtype(dtype).device(device));
    auto h = backbone->forward(x.reshape({rows * C, De})).view({rows, C, H});
    return heads->forward(h);
  }
};

TORCH_MODULE(ChannelContextMdn);

struct ChannelContextPlusGlobalMdnImpl : torch::nn::Module {
  int64_t De{0};
  int64_t Dg{0};
  int64_t Df{0};
  int64_t C{0};
  int64_t Hf{0};
  int64_t K{0};
  int64_t H{0};
  int64_t depth{0};
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};

  cuwacunu::wikimyei::inference::expected_value::mdn::Backbone backbone{
      nullptr};
  ChannelContextHeads heads{nullptr};

  ChannelContextPlusGlobalMdnImpl(int64_t De_, int64_t Dg_, int64_t Df_,
                                  int64_t C_, int64_t Hf_, int64_t K_,
                                  int64_t H_, int64_t depth_,
                                  torch::Dtype dtype_ = torch::kFloat32,
                                  torch::Device device_ = torch::kCPU)
      : De(De_), Dg(Dg_), Df(Df_), C(C_), Hf(Hf_), K(K_), H(H_), depth(depth_),
        dtype(dtype_), device(device_) {
    TORCH_CHECK(De > 0 && Dg > 0 && Df > 0 && C > 0 && Hf > 0 && K > 0 &&
                    H > 0 && depth >= 0,
                "[ChannelContextPlusGlobalMdn] invalid dimensions");
    cuwacunu::wikimyei::inference::expected_value::mdn::BackboneOptions bopts{
        De + Dg + C, H, depth};
    backbone = register_module(
        "backbone",
        cuwacunu::wikimyei::inference::expected_value::mdn::Backbone(bopts));
    heads = register_module("heads", ChannelContextHeads(C, Hf, Df, K, H));
    this->to(device, dtype);
  }

  cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut forward(
      const torch::Tensor &channel_context, const torch::Tensor &channel_mask,
      const torch::Tensor &global_context, const torch::Tensor &global_mask) {
    TORCH_CHECK(channel_context.defined() && channel_context.dim() == 3,
                "[ChannelContextPlusGlobalMdn] channel_context must be "
                "[B_node,C,De]");
    TORCH_CHECK(channel_mask.defined() && channel_mask.dim() == 2,
                "[ChannelContextPlusGlobalMdn] channel_mask must be "
                "[B_node,C]");
    TORCH_CHECK(global_context.defined() && global_context.dim() == 2,
                "[ChannelContextPlusGlobalMdn] global_context must be "
                "[B_node,Dg]");
    TORCH_CHECK(global_mask.defined() && global_mask.dim() == 1,
                "[ChannelContextPlusGlobalMdn] global_mask must be [B_node]");
    TORCH_CHECK(channel_context.size(1) == C && channel_context.size(2) == De,
                "[ChannelContextPlusGlobalMdn] channel context shape mismatch");
    TORCH_CHECK(global_context.size(0) == channel_context.size(0) &&
                    global_context.size(1) == Dg &&
                    global_mask.size(0) == channel_context.size(0) &&
                    channel_mask.size(0) == channel_context.size(0) &&
                    channel_mask.size(1) == C,
                "[ChannelContextPlusGlobalMdn] mask/context shape mismatch");

    const auto rows = channel_context.size(0);
    auto z =
        channel_context.to(torch::TensorOptions().dtype(dtype).device(device));
    auto z_mask = channel_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(device));
    auto g =
        global_context.to(torch::TensorOptions().dtype(dtype).device(device));
    auto g_mask = global_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(device));

    auto channel_finite_or_masked =
        torch::isfinite(z).logical_or(z_mask.unsqueeze(-1).logical_not());
    TORCH_CHECK(channel_finite_or_masked.all().template item<bool>(),
                "[ChannelContextPlusGlobalMdn] non-finite channel context "
                "under true channel mask");
    auto global_finite_or_masked =
        torch::isfinite(g).logical_or(g_mask.unsqueeze(-1).logical_not());
    TORCH_CHECK(global_finite_or_masked.all().template item<bool>(),
                "[ChannelContextPlusGlobalMdn] non-finite global context under "
                "true global mask");

    auto clean_z = torch::where(z_mask.unsqueeze(-1), z, torch::zeros_like(z));
    auto clean_g = torch::where(g_mask.unsqueeze(-1), g, torch::zeros_like(g));
    auto expanded_g = clean_g.unsqueeze(1).expand({rows, C, Dg});
    auto channel_id =
        torch::eye(C, torch::TensorOptions().dtype(dtype).device(device))
            .unsqueeze(0)
            .expand({rows, C, C});
    auto x = torch::cat({clean_z, expanded_g, channel_id}, /*dim=*/2);
    auto active_mask = z_mask.logical_and(g_mask.unsqueeze(-1));
    x = x.masked_fill(active_mask.logical_not().unsqueeze(-1), 0.0);
    auto h = backbone->forward(x.reshape({rows * C, De + Dg + C}))
                 .view({rows, C, H});
    h = h.masked_fill(active_mask.logical_not().unsqueeze(-1), 0.0);
    return heads->forward(h);
  }
};

TORCH_MODULE(ChannelContextPlusGlobalMdn);

[[nodiscard]] inline channel_mdn_input_t
make_channel_mdn_input(const torch::Tensor &channel_context,
                       const torch::Tensor &context_mask,
                       const torch::Tensor &future_node_features,
                       const torch::Tensor &future_node_mask,
                       const std::vector<int64_t> &target_coords,
                       const torch::Tensor &anchor_keys = {},
                       std::vector<std::string> node_ids = {}) {
  TORCH_CHECK(channel_context.defined() && channel_context.dim() == 4,
              "[channel_context_mdn] context must be [B,N,C,De]");
  TORCH_CHECK(context_mask.defined() && context_mask.dim() == 3,
              "[channel_context_mdn] context_mask must be [B,N,C]");
  TORCH_CHECK(future_node_features.defined() && future_node_features.dim() == 5,
              "[channel_context_mdn] future_node_features must be "
              "[B,C,Hf,N,Dx]");
  TORCH_CHECK(future_node_mask.defined() && future_node_mask.dim() == 5,
              "[channel_context_mdn] future_node_mask must be "
              "[B,C,Hf,N,Dx]");
  TORCH_CHECK(future_node_features.sizes() == future_node_mask.sizes(),
              "[channel_context_mdn] future feature/mask shape mismatch");

  const auto B = channel_context.size(0);
  const auto N = channel_context.size(1);
  const auto C = channel_context.size(2);
  const auto De = channel_context.size(3);
  TORCH_CHECK(context_mask.sizes() == torch::IntArrayRef({B, N, C}),
              "[channel_context_mdn] context_mask shape mismatch");
  TORCH_CHECK(future_node_features.size(0) == B &&
                  future_node_features.size(1) == C &&
                  future_node_features.size(3) == N,
              "[channel_context_mdn] future shape does not match context");

  const auto Hf = future_node_features.size(2);
  const auto Dx = future_node_features.size(4);
  channel_context_mdn_detail::validate_target_coords(target_coords, Dx);
  auto device = channel_context.device();
  auto context_mask_bool =
      context_mask.to(torch::kBool).to(device).contiguous();
  auto context_finite_or_masked =
      torch::isfinite(channel_context)
          .logical_or(context_mask_bool.unsqueeze(-1).logical_not());
  TORCH_CHECK(context_finite_or_masked.all().item<bool>(),
              "[channel_context_mdn] non-finite context value under true "
              "context mask");
  auto clean_context =
      torch::where(context_mask_bool.unsqueeze(-1), channel_context,
                   torch::zeros_like(channel_context));

  auto target_index =
      torch::tensor(target_coords,
                    torch::TensorOptions().dtype(torch::kInt64).device(device));

  auto future = future_node_features.to(channel_context.options())
                    .permute({0, 3, 1, 2, 4})
                    .contiguous(); // [B,N,C,Hf,Dx]
  auto selected_future = future.index_select(/*dim=*/4, target_index);
  auto selected_feature_mask = future_node_mask.to(torch::kBool)
                                   .to(device)
                                   .permute({0, 3, 1, 2, 4})
                                   .contiguous()
                                   .index_select(/*dim=*/4, target_index);
  auto future_finite_or_masked =
      torch::isfinite(selected_future)
          .logical_or(selected_feature_mask.logical_not());
  TORCH_CHECK(future_finite_or_masked.all().item<bool>(),
              "[channel_context_mdn] non-finite future value under true target "
              "mask");
  auto future_mask = selected_feature_mask.all(/*dim=*/4);
  auto selected_future_clean =
      torch::where(future_mask.unsqueeze(-1), selected_future,
                   torch::zeros_like(selected_future));

  auto index_opts = torch::TensorOptions().dtype(torch::kInt64).device(device);
  auto anchor_index =
      torch::arange(B, index_opts).unsqueeze(1).repeat({1, N}).reshape({B * N});
  auto node_index =
      torch::arange(N, index_opts).unsqueeze(0).repeat({B, 1}).reshape({B * N});

  channel_mdn_input_t out{};
  out.context = clean_context.contiguous().reshape({B * N, C, De});
  out.context_mask = context_mask_bool.reshape({B * N, C});
  out.future = selected_future_clean.contiguous().reshape(
      {B * N, C, Hf, static_cast<int64_t>(target_coords.size())});
  out.future_mask = future_mask.contiguous().reshape({B * N, C, Hf});
  out.anchor_index = std::move(anchor_index);
  out.node_index = std::move(node_index);
  out.anchor_keys = anchor_keys;
  out.target_coords = target_coords;
  out.node_ids = std::move(node_ids);
  return out;
}

[[nodiscard]] inline torch::Tensor compute_channel_context_mdn_nll(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out,
    const channel_mdn_input_t &input,
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnNllOptions
        &options = {}) {
  const auto combined_mask = combine_channel_context_and_future_mask(
      input.context_mask, input.future_mask);
  cuwacunu::wikimyei::inference::expected_value::mdn::MdnNLLLoss loss(options);
  return loss.compute(out, input.future, combined_mask);
}

[[nodiscard]] inline torch::Tensor compute_channel_context_plus_global_mdn_nll(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out,
    const channel_mdn_plus_global_input_t &input,
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnNllOptions
        &options = {}) {
  const auto combined_mask =
      combine_channel_plus_global_context_and_future_mask(
          input.channel.context_mask, input.global_mask,
          input.channel.future_mask);
  cuwacunu::wikimyei::inference::expected_value::mdn::MdnNLLLoss loss(options);
  return loss.compute(out, input.channel.future, combined_mask);
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
