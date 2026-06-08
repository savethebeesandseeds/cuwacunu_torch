// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/assembly.h"
#include "wikimyei/policy/portfolio/graph_node_allocation/spec.h"

namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation {

inline constexpr const char *k_graph_node_allocation_torch_policy_module_v0 =
    "wikimyei.policy.portfolio.graph_node_allocation.torch_policy_module.v0";

struct graph_node_allocation_torch_policy_options_t {
  graph_node_allocation_net_spec_t net{};
  torch::Dtype dtype{torch::kFloat64};
  torch::Device device{torch::kCPU};
};

struct graph_node_allocation_torch_policy_output_t {
  torch::Tensor node_weight_logits{};         // [A]
  torch::Tensor action_distribution_params{}; // [1]
  torch::Tensor state_value{};                // scalar or [1]
};

namespace torch_policy_detail {

inline void validate_tensor_shape(const torch::Tensor &tensor,
                                  std::int64_t dim0, std::int64_t dim1,
                                  const char *name) {
  if (!tensor.defined() || tensor.dim() != 2 || tensor.size(0) != dim0 ||
      tensor.size(1) != dim1 ||
      !torch::isfinite(tensor.to(torch::kFloat64)).all().item<bool>()) {
    throw std::runtime_error(std::string("[graph_node_allocation_torch] ") +
                             name + " must be finite [A,F]");
  }
}

inline void validate_vector_shape(const torch::Tensor &tensor,
                                  std::int64_t size, const char *name) {
  if (!tensor.defined() || tensor.dim() != 1 || tensor.size(0) != size ||
      !torch::isfinite(tensor.to(torch::kFloat64)).all().item<bool>()) {
    throw std::runtime_error(std::string("[graph_node_allocation_torch] ") +
                             name + " must be finite [F]");
  }
}

inline void validate_mask_shape(const torch::Tensor &mask, std::int64_t A) {
  if (!mask.defined() || mask.dim() != 1 || mask.size(0) != A ||
      mask.scalar_type() != torch::kBool) {
    throw std::runtime_error(
        "[graph_node_allocation_torch] executable_mask must be bool [A]");
  }
  if (mask.to(torch::kInt64).sum().item<std::int64_t>() <= 0) {
    throw std::runtime_error(
        "[graph_node_allocation_torch] at least one executable node is "
        "required");
  }
}

[[nodiscard]] inline std::string
architecture_digest(const graph_node_allocation_net_spec_t &net) {
  using cuwacunu::wikimyei::assembly::assembly_detail::hash_hex;
  using cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  using cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string;
  std::uint64_t hash = kFnvOffsetBasis;
  mix_hash_string(hash, "wikimyei.policy.portfolio.graph_node_allocation."
                        "torch_policy_module.architecture.v0");
  mix_hash_string(hash, net.version_token);
  mix_hash_string(hash, net.policy_input_schema);
  mix_hash_string(hash, net.policy_input_feature_manifest);
  mix_hash_string(hash, std::to_string(net.input_node_feature_dim));
  mix_hash_string(hash, std::to_string(net.input_global_feature_dim));
  mix_hash_string(hash, std::to_string(net.input_risk_feature_dim));
  mix_hash_string(hash, std::to_string(net.node_encoder_layers));
  mix_hash_string(hash, std::to_string(net.node_encoder_hidden_dim));
  mix_hash_string(hash, std::to_string(net.global_encoder_layers));
  mix_hash_string(hash, std::to_string(net.global_encoder_hidden_dim));
  mix_hash_string(hash, std::to_string(net.risk_encoder_layers));
  mix_hash_string(hash, std::to_string(net.risk_encoder_hidden_dim));
  mix_hash_string(hash, net.pooling);
  mix_hash_string(hash, std::to_string(net.fusion_hidden_dim));
  mix_hash_string(hash, std::to_string(net.fusion_layers));
  mix_hash_string(hash, net.activation);
  mix_hash_string(hash, net.normalization);
  mix_hash_string(hash, std::to_string(net.policy_head_hidden_dim));
  mix_hash_string(hash, std::to_string(net.value_head_hidden_dim));
  mix_hash_string(hash, net.output_head);
  mix_hash_string(hash, net.value_head);
  mix_hash_string(hash, net.action_distribution);
  mix_hash_string(hash, net.dirichlet_concentration_head);
  mix_hash_string(hash, net.logistic_normal_candidate);
  return hash_hex(hash);
}

} // namespace torch_policy_detail

class GraphNodeAllocationMLPImpl : public torch::nn::Module {
public:
  GraphNodeAllocationMLPImpl(std::int64_t input_dim, std::int64_t hidden_dim,
                             std::int64_t output_dim, std::int64_t layer_count,
                             bool final_activation)
      : final_activation_(final_activation) {
    if (input_dim <= 0 || hidden_dim <= 0 || output_dim <= 0 ||
        layer_count <= 0) {
      throw std::runtime_error(
          "[graph_node_allocation_mlp] invalid dimensions");
    }
    for (std::int64_t layer = 0; layer < layer_count; ++layer) {
      const std::int64_t in = layer == 0 ? input_dim : hidden_dim;
      const bool final_layer = layer + 1 == layer_count;
      const std::int64_t out = final_layer ? output_dim : hidden_dim;
      auto linear = register_module("linear_" + std::to_string(layer),
                                    torch::nn::Linear(in, out));
      layers_.push_back(linear);
      if (!final_layer || final_activation_) {
        auto norm = register_module(
            "norm_" + std::to_string(layer),
            torch::nn::LayerNorm(torch::nn::LayerNormOptions({out})));
        norms_.push_back(norm);
      }
    }
  }

  [[nodiscard]] torch::Tensor forward(torch::Tensor x) {
    std::size_t norm_index = 0;
    for (std::size_t i = 0; i < layers_.size(); ++i) {
      const bool final_layer = i + 1 == layers_.size();
      x = layers_[i]->forward(x);
      if (!final_layer || final_activation_) {
        x = norms_[norm_index++]->forward(x);
        x = torch::silu(x);
      }
    }
    return x;
  }

private:
  bool final_activation_{false};
  std::vector<torch::nn::Linear> layers_{};
  std::vector<torch::nn::LayerNorm> norms_{};
};

TORCH_MODULE(GraphNodeAllocationMLP);

class GraphNodeAllocationTorchPolicyModuleImpl : public torch::nn::Module {
public:
  explicit GraphNodeAllocationTorchPolicyModuleImpl(
      graph_node_allocation_torch_policy_options_t options)
      : options_(std::move(options)) {
    validate_graph_node_allocation_net_spec(options_.net);

    node_encoder_ = register_module(
        "node_encoder",
        GraphNodeAllocationMLP(options_.net.input_node_feature_dim,
                               options_.net.node_encoder_hidden_dim,
                               options_.net.node_encoder_hidden_dim,
                               options_.net.node_encoder_layers,
                               /*final_activation=*/true));
    global_encoder_ = register_module(
        "global_encoder",
        GraphNodeAllocationMLP(options_.net.input_global_feature_dim,
                               options_.net.global_encoder_hidden_dim,
                               options_.net.global_encoder_hidden_dim,
                               options_.net.global_encoder_layers,
                               /*final_activation=*/true));
    risk_encoder_ = register_module(
        "risk_encoder",
        GraphNodeAllocationMLP(options_.net.input_risk_feature_dim,
                               options_.net.risk_encoder_hidden_dim,
                               options_.net.risk_encoder_hidden_dim,
                               options_.net.risk_encoder_layers,
                               /*final_activation=*/true));

    const std::int64_t per_node_fusion_input =
        options_.net.node_encoder_hidden_dim * 3 +
        options_.net.global_encoder_hidden_dim +
        options_.net.risk_encoder_hidden_dim;
    fusion_ = register_module(
        "fusion", GraphNodeAllocationMLP(per_node_fusion_input,
                                         options_.net.fusion_hidden_dim,
                                         options_.net.fusion_hidden_dim,
                                         options_.net.fusion_layers,
                                         /*final_activation=*/true));
    policy_head_hidden_ =
        register_module("policy_head_hidden",
                        torch::nn::Linear(options_.net.fusion_hidden_dim,
                                          options_.net.policy_head_hidden_dim));
    policy_head_norm_ = register_module(
        "policy_head_norm", torch::nn::LayerNorm(torch::nn::LayerNormOptions(
                                {options_.net.policy_head_hidden_dim})));
    policy_head_out_ =
        register_module("policy_head_out",
                        torch::nn::Linear(options_.net.policy_head_hidden_dim,
                                          /*out_features=*/1));

    const std::int64_t value_input = options_.net.node_encoder_hidden_dim * 2 +
                                     options_.net.global_encoder_hidden_dim +
                                     options_.net.risk_encoder_hidden_dim;
    value_head_hidden_ = register_module(
        "value_head_hidden",
        torch::nn::Linear(value_input, options_.net.value_head_hidden_dim));
    value_head_norm_ = register_module(
        "value_head_norm", torch::nn::LayerNorm(torch::nn::LayerNormOptions(
                               {options_.net.value_head_hidden_dim})));
    value_head_out_ = register_module(
        "value_head_out", torch::nn::Linear(options_.net.value_head_hidden_dim,
                                            /*out_features=*/1));
    concentration_head_hidden_ = register_module(
        "concentration_head_hidden",
        torch::nn::Linear(value_input, options_.net.policy_head_hidden_dim));
    concentration_head_norm_ =
        register_module("concentration_head_norm",
                        torch::nn::LayerNorm(torch::nn::LayerNormOptions(
                            {options_.net.policy_head_hidden_dim})));
    concentration_head_out_ =
        register_module("concentration_head_out",
                        torch::nn::Linear(options_.net.policy_head_hidden_dim,
                                          /*out_features=*/1));

    this->to(options_.device, options_.dtype);
  }

  [[nodiscard]] const graph_node_allocation_torch_policy_options_t &
  options() const {
    return options_;
  }

  [[nodiscard]] std::string architecture_digest() const {
    return torch_policy_detail::architecture_digest(options_.net);
  }

  [[nodiscard]] graph_node_allocation_torch_policy_output_t
  forward(const torch::Tensor &node_features,
          const torch::Tensor &global_features,
          const torch::Tensor &risk_features,
          const torch::Tensor &executable_mask) {
    const auto A = node_features.defined() && node_features.dim() == 2
                       ? node_features.size(0)
                       : -1;
    torch_policy_detail::validate_tensor_shape(
        node_features, A, options_.net.input_node_feature_dim, "node_features");
    torch_policy_detail::validate_vector_shape(
        global_features, options_.net.input_global_feature_dim,
        "global_features");
    torch_policy_detail::validate_vector_shape(
        risk_features, options_.net.input_risk_feature_dim, "risk_features");
    torch_policy_detail::validate_mask_shape(executable_mask, A);

    auto node_x = node_features.to(
        torch::TensorOptions().dtype(options_.dtype).device(options_.device));
    auto global_x = global_features.to(
        torch::TensorOptions().dtype(options_.dtype).device(options_.device));
    auto risk_x = risk_features.to(
        torch::TensorOptions().dtype(options_.dtype).device(options_.device));
    auto mask = executable_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(options_.device));

    const auto node_embedding = node_encoder_->forward(node_x);
    const auto global_embedding = global_encoder_->forward(global_x);
    const auto risk_embedding = risk_encoder_->forward(risk_x);
    const auto mask_f = mask.to(options_.dtype).unsqueeze(1);
    const auto active_count = mask_f.sum().clamp_min(1.0);
    const auto mean_pool =
        (node_embedding * mask_f).sum(/*dim=*/0) / active_count;
    const auto masked_for_max =
        torch::where(mask.unsqueeze(1), node_embedding,
                     torch::full_like(node_embedding, -1.0e30));
    const auto max_pool = std::get<0>(masked_for_max.max(/*dim=*/0));

    const auto global_expand = global_embedding.unsqueeze(0).expand({A, -1});
    const auto risk_expand = risk_embedding.unsqueeze(0).expand({A, -1});
    const auto mean_expand = mean_pool.unsqueeze(0).expand({A, -1});
    const auto max_expand = max_pool.unsqueeze(0).expand({A, -1});
    const auto per_node = torch::cat(
        {node_embedding, global_expand, risk_expand, mean_expand, max_expand},
        /*dim=*/1);
    const auto fused = fusion_->forward(per_node);

    auto policy_hidden = policy_head_hidden_->forward(fused);
    policy_hidden = torch::silu(policy_head_norm_->forward(policy_hidden));
    auto logits = policy_head_out_->forward(policy_hidden).squeeze(-1);

    const auto value_context =
        torch::cat({global_embedding, risk_embedding, mean_pool, max_pool},
                   /*dim=*/0)
            .unsqueeze(0);
    auto value_hidden = value_head_hidden_->forward(value_context);
    value_hidden = torch::silu(value_head_norm_->forward(value_hidden));
    auto value = value_head_out_->forward(value_hidden).reshape({1});

    auto concentration_hidden =
        concentration_head_hidden_->forward(value_context);
    concentration_hidden =
        torch::silu(concentration_head_norm_->forward(concentration_hidden));
    auto concentration =
        concentration_head_out_->forward(concentration_hidden).reshape({1});

    return {.node_weight_logits = logits.to(torch::kFloat64).contiguous(),
            .action_distribution_params =
                concentration.to(torch::kFloat64).contiguous(),
            .state_value = value.to(torch::kFloat64).contiguous()};
  }

private:
  graph_node_allocation_torch_policy_options_t options_{};
  GraphNodeAllocationMLP node_encoder_{nullptr};
  GraphNodeAllocationMLP global_encoder_{nullptr};
  GraphNodeAllocationMLP risk_encoder_{nullptr};
  GraphNodeAllocationMLP fusion_{nullptr};
  torch::nn::Linear policy_head_hidden_{nullptr};
  torch::nn::LayerNorm policy_head_norm_{nullptr};
  torch::nn::Linear policy_head_out_{nullptr};
  torch::nn::Linear value_head_hidden_{nullptr};
  torch::nn::LayerNorm value_head_norm_{nullptr};
  torch::nn::Linear value_head_out_{nullptr};
  torch::nn::Linear concentration_head_hidden_{nullptr};
  torch::nn::LayerNorm concentration_head_norm_{nullptr};
  torch::nn::Linear concentration_head_out_{nullptr};
};

TORCH_MODULE(GraphNodeAllocationTorchPolicyModule);

[[nodiscard]] inline graph_node_allocation_torch_policy_options_t
make_graph_node_allocation_torch_policy_options(
    graph_node_allocation_net_spec_t net = {}) {
  validate_graph_node_allocation_net_spec(net);
  graph_node_allocation_torch_policy_options_t out{};
  out.net = std::move(net);
  out.dtype = torch::kFloat64;
  out.device = torch::kCPU;
  return out;
}

} // namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation
