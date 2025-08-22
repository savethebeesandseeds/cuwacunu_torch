/* jk_losses.h
 *
 * Lightweight loss wrappers + an output "view" for passing model outputs
 * without forcing a specific struct type. Designed to avoid dictionary
 * lookups and to keep loss code decoupled from model classes.
 *
 * Notes:
 *  - OutView stores *pointers* to tensors. Make sure the referenced tensors
 *    outlive the loss call (i.e., don't pass temporaries).
 *  - For MDN: sigma is interpreted as per-dimension **standard deviation**
 *    (not variance). Shapes assumed:
 *        log_pi: [B, K]
 *        mu    : [B, K, D]
 *        sigma : [B, K, D]   (σ > 0)
 *        target: [B, D]
 *  - CrossEntropyLoss expects target as class indices (Long dtype) by default.
 *  - BCEWithLogitsLoss expects raw logits; target is usually float in [0,1].
 */

#pragma once

#include <torch/torch.h>
#include <torch/nn/functional/loss.h>
#include <cassert>
#include <cmath>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace jkimyei {

// --- VICReg helpers ---
inline at::Tensor off_diagonal(const at::Tensor& m) {
  const auto S = m.size(0);
  TORCH_CHECK(S == m.size(1), "[VICReg] off_diagonal expects a square matrix");
  return m.flatten().slice(/*dim=*/0, /*start=*/0, /*end=*/S * S - 1)
          .view({S - 1, S + 1})
          .slice(/*dim=*/1, /*start=*/1, /*end=*/S + 1)
          .flatten();
}

/* A light-weight "view" of model outputs.
 * Fill only the slots your model produces and set the bitmask accordingly.
 */
struct OutView {
  // Optional slots (filled by each model as needed)
  const at::Tensor* logits = nullptr;  // classifiers: raw logits
  const at::Tensor* pred   = nullptr;  // regressors: predicted values/scores
  const at::Tensor* log_pi = nullptr;  // MDN: mixture log-weights [B,K]
  const at::Tensor* mu     = nullptr;  // MDN: means [B,K,D]
  const at::Tensor* sigma  = nullptr;  // MDN: stddevs [B,K,D]

  enum Bits : uint32_t { Logits=1u<<0, Pred=1u<<1, LogPi=1u<<2, Mu=1u<<3, Sigma=1u<<4 };
  uint32_t mask = 0;

  inline bool has(uint32_t b) const { return (mask & b) != 0; }

  // Convenience builders
  static inline OutView from_mdn(const at::Tensor& log_pi,
                                 const at::Tensor& mu,
                                 const at::Tensor& sigma) {
    OutView v; v.log_pi=&log_pi; v.mu=&mu; v.sigma=&sigma; v.mask=LogPi|Mu|Sigma; return v;
  }
  static inline OutView from_pred(const at::Tensor& pred_) {
    OutView v; v.pred=&pred_; v.mask=Pred; return v;
  }
  static inline OutView from_logits(const at::Tensor& logits_) {
    OutView v; v.logits=&logits_; v.mask=Logits; return v;
  }
};

/* Loss interface: implement operator() to compute a scalar */
struct ILoss {
  virtual ~ILoss() = default;

  // 1) OutView entry. Default: forward to tensor overload if `pred` exists.
  virtual at::Tensor operator()(const OutView& out,
                                const at::Tensor& target) const {
    // Adjust these to your OutView API
    TORCH_CHECK(out.has(OutView::Pred),
      "ILoss(OutView,...): no 'pred' in OutView; either override this "
      "overload or provide a tensor-based overload and include 'pred'.");
    return (*this)(*out.pred, target);  // calls the tensor overload (virtual)
  }

  // 2) Tensor entry. Default: complain unless a derived class provides it.
  virtual at::Tensor operator()(const at::Tensor& pred,
                                const at::Tensor& target) const {
    TORCH_CHECK(false,
      "ILoss(Tensor,Tensor): not implemented. Override in derived class.");
  }
};

/* MDN Negative Log-Likelihood (diagonal Gaussians)
 *
 * log N(x|μ, Σ) = -0.5[(x-μ)^T Σ^{-1}(x-μ) + D log(2π) + log|Σ|]
 * For diagonal Σ = diag(σ^2), log|Σ| = 2 * sum(log σ).
 * We implement log p(x) = logsumexp_k ( log π_k + log N_k(x) ).
 */
struct MdnNllLoss final : ILoss {
  at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
    TORCH_CHECK(out.has(OutView::LogPi|OutView::Mu|OutView::Sigma),
                "MdnNllLoss needs log_pi, mu, sigma");
    const auto& log_pi = *out.log_pi; // [B,K]
    const auto& mu     = *out.mu;     // [B,K,D]
    const auto& sigma  = *out.sigma;  // [B,K,D]

    TORCH_CHECK(target.dim() == 2, "target must be [B,D]");
    TORCH_CHECK(mu.dim()==3 && sigma.dim()==3, "mu/sigma must be [B,K,D]");
    TORCH_CHECK(mu.sizes() == sigma.sizes(), "mu/sigma shape mismatch");
    TORCH_CHECK(log_pi.sizes()[0] == mu.sizes()[0] && log_pi.sizes()[1] == mu.sizes()[1],
                "log_pi must be [B,K] matching mu/sigma batch and K");

    const auto B = target.size(0);
    const auto K = mu.size(1);
    const auto D = mu.size(2);
    TORCH_CHECK(target.size(1) == D, "target D must match mu/sigma D");

    // Broadcast target to [B,K,D]
    auto x = target.unsqueeze(1).expand({B, K, D});

    // Numerical guard; keep everything on the same device/dtype
    constexpr double kEps = 1e-12;
    auto eps = torch::tensor(kEps, sigma.options());
    auto inv_var = (sigma * sigma + eps).reciprocal();                  // [B,K,D]
    auto quad    = ((x - mu).pow(2) * inv_var).sum(-1);                 // [B,K]
    auto log_det_sigma = sigma.clamp_min(kEps).log().sum(-1);           // [B,K]

    // -0.5 * (quad + D*log(2π)) - 0.5*log|Σ|
    // with log|Σ| = 2 * sum(log σ)  => subtracting sum(log σ) is correct.
    const double two_pi = 2.0 * M_PI;
    auto log_comp = log_pi
                  - 0.5 * (quad + D * std::log(two_pi))
                  - log_det_sigma;                                      // [B,K]

    auto log_p = torch::logsumexp(log_comp, /*dim=*/1);                 // [B]
    return -log_p.mean();                                               // scalar
  }
};

/* Standard regression losses */
struct MSELoss final : ILoss {
  at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
    TORCH_CHECK(out.has(OutView::Pred), "MSELoss expects pred");
    return torch::mse_loss(*out.pred, target, at::Reduction::Mean);
  }
};

struct L1Loss final : ILoss {
  at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
    TORCH_CHECK(out.has(OutView::Pred), "L1Loss expects pred");
    return torch::l1_loss(*out.pred, target, at::Reduction::Mean);
  }
};

/* Classification losses */
struct CrossEntropyLoss final : ILoss {
  explicit CrossEntropyLoss(double label_smoothing=0.0)
  : label_smoothing_(label_smoothing) {}

  at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
    TORCH_CHECK(out.has(OutView::Logits), "CrossEntropyLoss expects logits");
    // target is class indices (Long) or prob dists depending on F.cross_entropy semantics
    using namespace torch::nn::functional;
    CrossEntropyFuncOptions opts;
    if (label_smoothing_ != 0.0) {
      opts = opts.label_smoothing(label_smoothing_);
    }
    return cross_entropy(*out.logits, target, opts);
  }
private:
  double label_smoothing_;
};

/* VICReg loss (Invariance + Variance + Covariance)
 * Expects two representations with identical shape. We take:
 *   x := out.pred
 *   y := target
 * Shapes can be [B,E] or any [..., E]; they will be flattened to [B, E].
 */
struct VicRegLoss final : ILoss {
  explicit VicRegLoss(double sim_coeff=25.0, double std_coeff=25.0, double cov_coeff=1.0)
  : sim_coeff_(sim_coeff), std_coeff_(std_coeff), cov_coeff_(cov_coeff) {}

  at::Tensor operator()(const at::Tensor& pred, const at::Tensor& target) const override {
    const auto& x_raw = pred;        // representation 1
    const auto& y_raw = target;      // representation 2

    TORCH_CHECK(x_raw.sizes().back() == y_raw.sizes().back(),
                "[VICReg] last dim (embedding) must match");
    TORCH_CHECK(x_raw.numel() > 0 && y_raw.numel() > 0,
                "[VICReg] empty tensors");

    // Flatten leading dims so we get [B, E]
    auto x = x_raw.flatten(0, x_raw.dim() - 2);
    auto y = y_raw.flatten(0, y_raw.dim() - 2);

    TORCH_CHECK(x.sizes() == y.sizes(), "[VICReg] x and y must have same shape");
    const int64_t B = x.size(0);
    const int64_t E = x.size(1);
    TORCH_CHECK(B > 1, "[VICReg] covariance requires at least 2 samples (B>1)");

    // Keep everything on x's device/dtype
    const auto opts = x.options();

    // Invariance (MSE between views)
    auto repr_loss = torch::mse_loss(x, y, at::Reduction::Mean);

    // Mean-center for var/cov terms
    x = x - x.mean(0);
    y = y - y.mean(0);

    // Variance term: encourage each dim's std >= 1
    auto eps     = torch::tensor(1e-4, opts);
    auto one     = torch::tensor(1.0,  opts);
    auto half    = torch::tensor(0.5,  opts);

    auto std_x = torch::sqrt(x.var(/*dim=*/0, /*unbiased=*/false) + eps);
    auto std_y = torch::sqrt(y.var(/*dim=*/0, /*unbiased=*/false) + eps);

    auto std_loss = half * (torch::mean(torch::relu(one - std_x)) +
                            torch::mean(torch::relu(one - std_y)));

    // Covariance term: penalize off-diagonal covariance
    auto denom   = torch::tensor(static_cast<double>(B - 1), opts);
    auto e_sz    = torch::tensor(static_cast<double>(E),     opts);

    auto cov_x = x.t().matmul(x) / denom;     // [E,E]
    auto cov_y = y.t().matmul(y) / denom;

    auto cov_loss = (off_diagonal(cov_x).pow_(2).sum() +
                     off_diagonal(cov_y).pow_(2).sum()) / e_sz;

    // Weighted sum (all terms are scalar on same device)
    auto sc = torch::tensor(sim_coeff_, opts);
    auto vc = torch::tensor(std_coeff_, opts);
    auto cc = torch::tensor(cov_coeff_, opts);

    return sc * repr_loss + vc * std_loss + cc * cov_loss;
  }

private:
  double sim_coeff_;
  double std_coeff_;
  double cov_coeff_;
};

struct BCEWithLogitsLoss final : ILoss {
  explicit BCEWithLogitsLoss(double pos_weight = 1.0) : pos_weight_(pos_weight) {}

  at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
    assert(out.has(OutView::Logits));
    namespace F = torch::nn::functional;
    F::BinaryCrossEntropyWithLogitsFuncOptions opts;
    // pos_weight must be a Tensor (broadcastable to logits). Scalar is fine.
    auto pw = torch::full({}, pos_weight_, out.logits->options());
    opts = opts.pos_weight(pw);
    // (optional) opts = opts.reduction(torch::kMean); // default is Mean already
    // Ensure target is floating (BCE expects float targets)
    auto tgt = target.dtype() == out.logits->dtype()
                 ? target
                 : target.to(out.logits->dtype());

    return F::binary_cross_entropy_with_logits(*out.logits, tgt, opts);
  }

private:
  double pos_weight_;
};
/* Robust regression (Huber) */
struct SmoothL1Loss final : ILoss {
  explicit SmoothL1Loss(double beta=1.0) : beta_(beta) {}

  at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
    TORCH_CHECK(out.has(OutView::Pred), "SmoothL1Loss expects pred");
    using namespace torch::nn::functional;
    SmoothL1LossFuncOptions opts;
    opts = opts.beta(beta_);
    return smooth_l1_loss(*out.pred, target, opts);
  }
private:
  double beta_;
};

/* Binary hinge embedding loss (targets in {-1, +1}) */
struct HingeEmbeddingLoss final : ILoss {
  explicit HingeEmbeddingLoss(double margin=1.0) : margin_(margin) {}

  at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
    TORCH_CHECK(out.has(OutView::Pred), "HingeEmbeddingLoss expects pred (scores)");
    using namespace torch::nn::functional;
    HingeEmbeddingLossFuncOptions opts;
    opts = opts.margin(margin_);
    return hinge_embedding_loss(*out.pred, target, opts);
  }
private:
  double margin_;
};
/* -------------------------- Row -> Builder ----------------------------- */

/* Map a config-row (BNF) to a concrete loss.
 * - Enforces exact columns: {row_id, loss_function_type, options}
 * - Enforces exact options per loss (no extras, no missing)
 */
inline std::unique_ptr<ILoss>
make_loss_from_row(const std::unordered_map<std::string,std::string>& row) {
  // Schema: exact columns, but allow options to be "-" for losses with no params.
  cuwacunu::camahjucunu::require_columns_exact(row, { ROW_ID_COLUMN_HEADER, "loss_function_type", "options" }, /*enforce_nonempty=*/false);

  const std::string type = cuwacunu::camahjucunu::require_column(row, "loss_function_type");

  // Helper: assert that options is empty/"-" (i.e., no key=value pairs)
  auto ensure_no_options = [&]() {
    auto it = row.find("options");
    if (it == row.end()) return;
    auto kv = cuwacunu::camahjucunu::parse_options_kvlist(it->second);
    if (!kv.empty()) {
      std::ostringstream oss;
      oss << "Unexpected options for loss_function_type='" << type << "'. None are allowed.";
      throw std::runtime_error(oss.str());
    }
  };

  if (type == "NLLLoss") {         // MDN-NLL (no configurable options)
    ensure_no_options();
    return std::make_unique<MdnNllLoss>();
  }

  if (type == "MeanSquaredError") { // MSE (no options)
    ensure_no_options();
    return std::make_unique<MSELoss>();
  }

  if (type == "L1Loss") {           // L1 (no options)
    ensure_no_options();
    return std::make_unique<L1Loss>();
  }

  if (type == "CrossEntropy") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "label_smoothing" });
    const double ls = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "label_smoothing"));
    return std::make_unique<CrossEntropyLoss>(ls);
  }

  if (type == "BinaryCrossEntropy") { // BCEWithLogits variant
    cuwacunu::camahjucunu::validate_options_exact(row, { "pos_weight" });
    const double pw = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "pos_weight"));
    return std::make_unique<BCEWithLogitsLoss>(pw);
  }

  if (type == "SmoothL1") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "beta" });
    const double beta = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "beta"));
    return std::make_unique<SmoothL1Loss>(beta);
  }

  if (type == "Hinge") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "margin" });
    const double margin = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "margin"));
    return std::make_unique<HingeEmbeddingLoss>(margin);
  }

  if (type == "VICReg") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "sim_coeff", "std_coeff", "cov_coeff" });
    const double simc = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "sim_coeff"));
    const double stdc = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "std_coeff"));
    const double covc = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "cov_coeff"));
    return std::make_unique<VicRegLoss>(simc, stdc, covc);
  }

  throw std::runtime_error("Unknown loss_function_type: " + type);
}


inline std::unique_ptr<ILoss> make_loss(const cuwacunu::camahjucunu::BNF::training_instruction_t& inst, const std::string& row_id) {
  const auto& row = inst.retrive_row("loss_functions_table", row_id);
  return make_loss_from_row(row);
}

} // namespace jkimyei
} // namespace cuwacunu
