/* mixture_density_network_loss.h */
#pragma once
#include <torch/torch.h>
#include <cctype>
#include <string>

#include "piaabo/dutils.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_utils.h"

#include "jkimyei/training_components/jk_setup.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

// ---------- Loss (configurable via jk_setup.loss_conf) ----------
// options (table row id == jk_setup.loss_conf.id):
//   eps=<float>        (default 1e-6)
//   sigma_min=<float>  (default 1e-3)
//   sigma_max=<float>  (default 0.0 → disabled)
//   reduction=<mean|sum> (default mean)
struct MdnNLLLoss {
  double eps_{1e-6};
  double sigma_min_{1e-3};
  double sigma_max_{0.0};
  bool   reduce_mean_{true};

  explicit MdnNLLLoss(const cuwacunu::jkimyei::jk_setup_t& jk_setup) {
    // Hard guard: the row in loss_functions_table should be of the expected type.
    ASSERT(jk_setup.loss_conf.type == "NLLLoss",
      ("Review <training_components>.instruction: MDN requires loss type 'NLLLoss', got '%s'." + jk_setup.loss_conf.type).c_str());

    // Options are optional. If the row or options are absent, defaults apply.
    try {
      const auto& row = jk_setup.inst.retrive_row("loss_functions_table", jk_setup.loss_conf.id);
      if (cuwacunu::camahjucunu::has_option(row, "eps"))
        eps_ = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "eps"));
      if (cuwacunu::camahjucunu::has_option(row, "sigma_min"))
        sigma_min_ = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "sigma_min"));
      if (cuwacunu::camahjucunu::has_option(row, "sigma_max"))
        sigma_max_ = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "sigma_max"));
      if (cuwacunu::camahjucunu::has_option(row, "reduction")) {
        std::string red = cuwacunu::camahjucunu::require_option(row, "reduction");
        for (auto& c : red) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        reduce_mean_ = (red != "sum"); // default mean; only "sum" switches it
      }
    } catch (...) {
      // keep defaults
    }
  }

  torch::Tensor operator()(const MdnOut& out, const torch::Tensor& y) const {
    // ---- Shape checks
    TORCH_CHECK(y.defined(), "[MdnNLLLoss] target y is undefined");
    TORCH_CHECK(out.log_pi.defined() && out.mu.defined() && out.sigma.defined(),
                "[MdnNLLLoss] output tensors must be defined");

    TORCH_CHECK(out.log_pi.dim() == 2, "[MdnNLLLoss] log_pi must be [B,K]");
    TORCH_CHECK(out.mu.dim() == 3     && out.sigma.dim() == 3,
                "[MdnNLLLoss] mu/sigma must be [B,K,Dy]");
    TORCH_CHECK(y.dim() == 2, "[MdnNLLLoss] y must be [B,Dy]");

    const auto B  = y.size(0);
    const auto Dy = y.size(1);
    TORCH_CHECK(out.mu.size(0) == B && out.sigma.size(0) == B && out.log_pi.size(0) == B,
                "[MdnNLLLoss] batch size mismatch");
    TORCH_CHECK(out.mu.size(2) == Dy && out.sigma.size(2) == Dy,
                "[MdnNLLLoss] target dimension Dy mismatch");
    TORCH_CHECK(out.log_pi.size(1) == out.mu.size(1) && out.mu.size(1) == out.sigma.size(1),
                "[MdnNLLLoss] K mismatch among log_pi/mu/sigma");

    // Broadcast y -> [B, K, Dy]
    auto y_b = y.unsqueeze(1).expand({B, out.mu.size(1), Dy}); // [B,K,Dy]

    // Clamp sigmas, keep numerics on the same device/dtype
    auto sigma = out.sigma.clamp_min(sigma_min_);
    if (sigma_max_ > 0.0) sigma = sigma.clamp_max(sigma_max_);
    auto eps_t = sigma.new_full({}, eps_); // scalar tensor, right dtype/device

    static const double LOG2PI = std::log(2.0 * M_PI);
    auto diff   = (y_b - out.mu) / (sigma + eps_t);
    auto quad   = -0.5 * diff.pow(2);
    auto logdet = -(sigma + eps_t).log();
    auto cterm  = -0.5 * LOG2PI;

    auto comp_logp = (quad + logdet + cterm).sum(-1);          // [B,K]
    auto log_mix   = logsumexp(out.log_pi + comp_logp, 1, false); // [B]
    auto nll       = -log_mix;

    return reduce_mean_ ? nll.mean() : nll.sum();
  }
};

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
