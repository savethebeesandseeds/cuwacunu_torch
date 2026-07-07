/* mixture_density_network_types.h */
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <torch/torch.h>

namespace cuwacunu {
namespace wikimyei {
namespace inference {
namespace expected_value {
namespace mdn {

// ---------- Output container ----------
// Active channel-context MDN contract:
// log_pi : [B, N, C, Df, K]
// mu     : [B, N, C, Df, K]
// sigma  : [B, N, C, Df, K]
// direct_edge_return : optional [B, N-1, C] direct base-minus-quote return
//                      readout for synthetic/edge diagnostics and auxiliary
//                      training.
//
// B  = anchor batch
// N  = graph node slot
// C  = channel slot
// Df = selected one-step target feature slot
// K  = mixture component
struct MdnOut {
  torch::Tensor log_pi;
  torch::Tensor mu;
  torch::Tensor sigma;
  torch::Tensor direct_edge_return;
};

struct ChannelAdapterOptions {
  int64_t feature_dim;  // H, backbone output width
  int64_t adapter_rank; // low-rank channel adapter width
};

struct FeatureConditionedMdnHeadOptions {
  int64_t feature_dim;           // H, adapted slot width
  int64_t target_feature_dim;    // Df, selected one-step target width
  int64_t mixture_count;         // K, mixture comps
  int64_t feature_embedding_dim; // Ef, learned target-feature identity width
  int64_t source_feature_vocab_size{
      0}; // embedding vocabulary for source coords
  std::vector<int64_t> target_coords{}; // source feature ids in output order
  double sigma_floor{1e-3};             // smooth model sigma floor
};

struct BackboneOptions {
  int64_t input_dim;   // De
  int64_t feature_dim; // H
  int64_t depth;       // residual blocks
};

struct DirectEdgeReturnHeadOptions {
  int64_t feature_dim{0};      // H, adapted slot width
  int64_t quote_node_index{0}; // v1 quote slot, currently expected to be 0
  std::string identity_mode{
      "shared"}; // shared|edge_embedding|per_edge|edge_embedding_per_edge
  int64_t base_edge_count{0}; // configured N-1 for identity/per-edge modes
  int64_t identity_embedding_dim{0};
  int64_t adapter_hidden_dim{
      0}; // optional residual adapter before edge features
};

struct residualOptions {
  int64_t in_dim;
  int64_t hidden;
};

struct InferenceConfig {};

} // namespace mdn
} // namespace expected_value
} // namespace inference
} // namespace wikimyei
} // namespace cuwacunu
