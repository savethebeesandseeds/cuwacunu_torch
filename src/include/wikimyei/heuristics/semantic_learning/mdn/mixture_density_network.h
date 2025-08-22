// Mixture Density Network (MDN) — LibTorch C++ skeleton 
// Single-file implementation with clean interfaces and 
// placeholders for training and inference routines.
//
// Design:
//   Namespace: cuwacunu::wikimyei::mdn
//   - Backbone: small MLP with residual blocks
//   - MdnHead: predicts (log_pi, mu, sigma) for diagonal Gaussian mixture
//   - MdnModel: composes Backbone + MdnHead
//   - Utilities: NLL, expectation, sampling (single-step), simple rollout
//   - Placeholders: train(...), inference(...)

#include <torch/torch.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

inline torch::Tensor mdn_nll(const MdnOut& out, const torch::Tensor& y);
inline torch::Tensor mdn_expectation(const MdnOut& out);
inline torch::Tensor mdn_sample_one_step(const MdnOut& out);
inline torch::Tensor rollout_sample_scalar(MdnModel& model, const torch::Tensor& x_seq, int64_t H);

// =============================
// Utility helpers
// =============================

inline torch::Tensor safe_softplus(const torch::Tensor& x, double eps = 1e-5) {
  return torch::softplus(x) + eps;
}

inline torch::Tensor logsumexp(const torch::Tensor& x, int64_t dim) {
  // numerically stable logsumexp
  auto max_x = std::get<0>(x.max(dim, true));
  return max_x + (x - max_x).exp().sum(dim, true).log();
}

// Gaussian log-pdf for diagonal covariance.
// y: [B, Dy]
// mu: [B, K, Dy]
// sigma: [B, K, Dy] (positive)
// returns log N(y; mu_k, diag(sigma_k^2)) as [B, K]
inline torch::Tensor diag_gaussian_logpdf(const torch::Tensor& y,
                                          const torch::Tensor& mu,
                                          const torch::Tensor& sigma) {
  TORCH_CHECK(y.dim() == 2, "y must be [B, Dy]");
  TORCH_CHECK(mu.dim() == 3 && sigma.dim() == 3, "mu/sigma must be [B, K, Dy]");
  auto B = y.size(0);
  auto Dy = y.size(1);
  TORCH_CHECK(mu.size(0) == B && sigma.size(0) == B, "batch mismatch");
  TORCH_CHECK(mu.size(2) == Dy && sigma.size(2) == Dy, "Dy mismatch");

  auto y_expanded = y.unsqueeze(1); // [B, 1, Dy]
  auto diff = (y_expanded - mu);    // [B, K, Dy]
  auto var = sigma * sigma;         // [B, K, Dy]
  auto log_det = (var).log().sum(-1); // [B, K]
  auto quad = (diff * diff / var).sum(-1); // [B, K]
  auto log2pi = std::log(2.0 * M_PI);
  auto log_prob = -0.5 * (Dy * log2pi + log_det + quad); // [B, K]
  return log_prob;
}

// =============================
// Residual MLP Block
// =============================
struct ResMlPBlockImpl : torch::nn::Module {
  torch::nn::Linear fc1{nullptr}, fc2{nullptr};
  torch::nn::LayerNorm ln1{nullptr}, ln2{nullptr};

  ResMlPBlockImpl(int64_t in_dim, int64_t hidden)
      : fc1(register_module("fc1", torch::nn::Linear(in_dim, hidden))),
        fc2(register_module("fc2", torch::nn::Linear(hidden, in_dim))),
        ln1(register_module("ln1", torch::nn::LayerNorm(torch::nn::LayerNormOptions(std::vector<int64_t>{in_dim})))),
        ln2(register_module("ln2", torch::nn::LayerNorm(torch::nn::LayerNormOptions(std::vector<int64_t>{in_dim})))) {}

  torch::Tensor forward(const torch::Tensor& x) {
    auto h = ln1->forward(x);
    h = torch::silu(fc1->forward(h));
    h = fc2->forward(h);
    auto y = x + h;
    y = torch::silu(ln2->forward(y));
    return y;
  }
};
TORCH_MODULE(ResMlPBlock);

// =============================
// Backbone: simple MLP with residuals
// =============================
struct BackboneOptions {
  int64_t input_dim;      // e.g., De (representation dim)
  int64_t feature_dim;    // e.g., 256/384/512
  int64_t depth;          // number of residual blocks (e.g., 2 or 3)
};

struct BackboneImpl : torch::nn::Module {
  torch::nn::Linear in{nullptr};
  std::vector<ResMlPBlock> blocks;
  torch::nn::LayerNorm out_norm{nullptr};

  BackboneImpl(const BackboneOptions& opt) {
    in = register_module("in", torch::nn::Linear(opt.input_dim, opt.feature_dim));
    blocks.reserve(opt.depth);
    for (int i = 0; i < opt.depth; ++i) {
      auto blk = ResMlPBlock(opt.feature_dim, opt.feature_dim * 2);
      blocks.push_back(register_module("blk" + std::to_string(i), blk));
    }
    out_norm = register_module("out_norm", torch::nn::LayerNorm(torch::nn::LayerNormOptions(std::vector<int64_t>{opt.feature_dim})));
  }

  torch::Tensor forward(const torch::Tensor& x) {
    auto h = torch::silu(in->forward(x));
    for (auto& blk : blocks) {
      h = blk->forward(h);
    }
    return out_norm->forward(h);
  }
};
TORCH_MODULE(Backbone);

// =============================
// MDN Head (diagonal Gaussian mixture)
// =============================
struct MdnHeadOptions {
  int64_t feature_dim; // from backbone
  int64_t Dy;          // target dimension (1 for your case)
  int64_t K;           // number of mixture components
};

struct MdnOut {
  torch::Tensor log_pi; // [B, K]
  torch::Tensor mu;     // [B, K, Dy]
  torch::Tensor sigma;  // [B, K, Dy] (positive)
};

struct MdnHeadImpl : torch::nn::Module {
  int64_t Dy, K;
  torch::nn::Linear lin_pi{nullptr}, lin_mu{nullptr}, lin_s{nullptr};

  MdnHeadImpl(const MdnHeadOptions& opt)
      : Dy(opt.Dy), K(opt.K) {
    lin_pi = register_module("lin_pi", torch::nn::Linear(opt.feature_dim, K));
    lin_mu = register_module("lin_mu", torch::nn::Linear(opt.feature_dim, K * Dy));
    lin_s  = register_module("lin_s",  torch::nn::Linear(opt.feature_dim, K * Dy));
  }

  MdnOut forward(const torch::Tensor& h) {
    auto B = h.size(0);
    auto log_pi = torch::log_softmax(lin_pi->forward(h), /*dim=*/-1); // [B, K]
    auto mu = lin_mu->forward(h).view({B, K, Dy});                    // [B, K, Dy]
    auto sigma = safe_softplus(lin_s->forward(h)).view({B, K, Dy});   // [B, K, Dy]
    return {log_pi, mu, sigma};
  }
};
TORCH_MODULE(MdnHead);

// =============================
// MDN Model
// =============================
struct MdnModelOptions {
  // Backbone
  int64_t input_dim;    // De
  int64_t feature_dim;  // e.g., 256
  int64_t depth;        // residual depth
  // Mixture
  int64_t Dy;           // 1 for scalar target
  int64_t K;            // components

  // ---- Training wiring (provided by your BNF outside the model) ----
  torch::optim::Optimizer& make_optimizer;

  // Loss delegate. If empty, defaults to mdn_nll.
  ... loss_fn;

  // Optional grad clip (<=0 disables)
  double grad_clip = 1.0;
};

struct MdnModelImpl : torch::nn::Module {
  Backbone backbone{nullptr};
  MdnHead head{nullptr};
  int64_t Dy, K;

  // training bits
  std::unique_ptr<torch::optim::Optimizer> optimizer_;
  std::function<torch::Tensor(const MdnOut&, const torch::Tensor&)> loss_fn_;
  double grad_clip_{1.0};

  MdnModelImpl(const MdnModelOptions& opt)
      : Dy(opt.Dy), K(opt.K) {
    // build network first
    BackboneOptions bopt{opt.input_dim, opt.feature_dim, opt.depth};
    backbone = register_module("backbone", Backbone(bopt));
    MdnHeadOptions hopt{opt.feature_dim, opt.Dy, opt.K};
    head = register_module("head", MdnHead(hopt));

    loss_fn_ = opt.loss_fn;
    optimizer_ = opt.optimizer;
    grad_clip_ = opt.grad_clip;
  }

  MdnOut forward(const torch::Tensor& x) {
    auto h = backbone->forward(x);
    return head->forward(h);
  }

  // keep cfg empty for now (you can extend later)
  struct TrainingConfig { /* empty on purpose */ };

  // single step — caller owns the loop
  torch::Tensor train_step(const torch::Tensor& x,   // [B, Dx]
                           const torch::Tensor& y,   // [B, Dy]
                           const TrainingConfig& /*cfg*/ = TrainingConfig{}) {
    TORCH_CHECK(optimizer_, "optimizer_ is null; did you pass make_optimizer in MdnModelOptions?");
    TORCH_CHECK(loss_fn_,   "loss_fn_ is null; pass loss_fn in MdnModelOptions or rely on default.");

    this->train();
    optimizer_->zero_grad();

    auto out  = this->forward(x);
    auto loss = loss_fn_(out, y);       // scalar

    loss.backward();

    if (grad_clip_ > 0.0) {
      torch::nn::utils::clip_grad_norm_(this->parameters(), grad_clip_);
    }

    optimizer_->step();

    return loss.detach();
  }

  // inference left for later
  // No default argument to avoid GCC nested-type quirk.
  std::vector<torch::Tensor> inference(/* features */
                                       const torch::Tensor& /*x0*/, // [B, Dx]
                                       const InferenceConfig& /*cfg*/) {
    // TODO: implement single-/multi-step sampling with iterative feeding.
    // Return shape suggestion: vector length = cfg.n_samples; each [B, horizon]
    return {};
  }
};
TORCH_MODULE(MdnModel);

// =============================
// Loss & metrics
// =============================
// MDN negative log-likelihood (diagonal Gaussian mixture)
// out: MdnOut {log_pi [B,K], mu [B,K,Dy], sigma [B,K,Dy]}
// y: [B, Dy]
inline torch::Tensor mdn_nll(const MdnOut& out, const torch::Tensor& y) {
  auto log_comp = diag_gaussian_logpdf(y, out.mu, out.sigma); // [B, K]
  auto log_mix = out.log_pi + log_comp;                       // [B, K]
  auto lse = logsumexp(log_mix, /*dim=*/1);                   // [B, 1]
  auto nll = -(lse.squeeze(1)).mean();                        // scalar
  return nll;
}

// Mixture expectation E[y|x], diagonal case
// returns [B, Dy]
inline torch::Tensor mdn_expectation(const MdnOut& out) {
  // softmax already applied; out.log_pi are log-weights
  auto pi = out.log_pi.exp().unsqueeze(-1); // [B, K, 1]
  auto e = (pi * out.mu).sum(1);            // [B, Dy]
  return e;
}

// One-step sampling: draws y_hat ~ mixture for each batch element.
// Returns [B, Dy]
inline torch::Tensor mdn_sample_one_step(const MdnOut& out) {
  auto device = out.log_pi.device();
  auto B = out.log_pi.size(0);
  auto Dy = out.mu.size(2);

  // Sample component indices per batch by categorical(pi)
  auto pi = out.log_pi.exp(); // [B,K]
  auto k_idx = torch::multinomial(pi, /*num_samples=*/1, /*replacement=*/true).squeeze(-1); // [B]

  // Gather mu and sigma for chosen components
  auto idx = k_idx.view({B, 1, 1}).expand({B, 1, Dy});
  auto mu_sel = out.mu.gather(1, idx).squeeze(1);     // [B, Dy]
  auto sig_sel = out.sigma.gather(1, idx).squeeze(1); // [B, Dy]

  // Sample standard normal and scale/shift
  auto eps = torch::randn(mu_sel.sizes(), mu_sel.options().device(device));
  auto y = mu_sel + sig_sel * eps; // [B, Dy]
  return y;
}

// Optional: short rollout utility for scalar Dy=1, no explicit lags.
// x_seq: [T, B, Dx]   (feature vectors per time-step)
// horizon H == T assumed for simplicity here; adapt as needed.
inline torch::Tensor rollout_sample_scalar(MdnModel& model,
                                           const torch::Tensor& x_seq,
                                           int64_t H) {
  TORCH_CHECK(x_seq.dim() == 3, "x_seq must be [T,B,Dx]");
  auto T = x_seq.size(0);
  TORCH_CHECK(H <= T, "H must be <= T for this simple helper");

  std::vector<torch::Tensor> preds;
  preds.reserve(H);

  for (int64_t t = 0; t < H; ++t) {
    // Assume x_seq[t] already contains the features for step t.
    auto out = model->forward(x_seq[t]); // [B,Dx] -> MdnOut
    auto y_t = mdn_sample_one_step(out); // [B,1]
    preds.push_back(y_t.squeeze(-1)); // [B]
  }
  auto Y = torch::stack(preds, /*dim=*/0); // [H, B]
  return Y;
}

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
