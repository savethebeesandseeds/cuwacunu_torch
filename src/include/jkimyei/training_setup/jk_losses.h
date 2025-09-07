/* jk_losses.h */

#pragma once

#include <torch/torch.h>
#include <torch/nn/functional/loss.h>
#include <cassert>
#include <cmath>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace jkimyei {
/* Map a config-row (BNF) to a concrete loss.
 * - Enforces exact columns: {row_id, type, options}
 * - Enforces exact options per loss (no extras, no missing)
 */
inline void validate_loss(const cuwacunu::camahjucunu::training_instruction_t& inst,
                          const std::string& row_id) {
  const auto& row = inst.retrive_row("loss_functions_table", row_id);

  // Exact columns; allow empties.
  cuwacunu::camahjucunu::require_columns_exact(
      row, { ROW_ID_COLUMN_HEADER, "type", "options" }, /*enforce_nonempty=*/false);

  const std::string type = cuwacunu::camahjucunu::require_column(row, "type");

  auto ensure_no_options = [&]() {
    auto it = row.find("options");
    if (it == row.end()) return;                  // column exists per require_columns_exact, but be safe
    std::string s = it->second;

    // Trim simple whitespace (no dependency on your utils)
    auto l = s.find_first_not_of(" \t\r\n");
    auto r = s.find_last_not_of(" \t\r\n");
    if (l == std::string::npos) s.clear();
    else                        s = s.substr(l, r - l + 1);

    if (s.empty() || s == "-") return;            // treat "" or "-" as "no options"

    auto kv = cuwacunu::camahjucunu::parse_options_kvlist(s);
    if (!kv.empty()) {
      std::ostringstream oss;
      oss << "Unexpected options for loss_function type='" << type << "'. None are allowed.";
      throw std::runtime_error(oss.str());
    }
  };

  if (type == "NLLLoss") { // MDN-NLL (no configurable options)
    cuwacunu::camahjucunu::validate_options_exact(row, { "eps", "sigma_min", "sigma_max", "reduction" });
    
  } else if (type == "MeanSquaredError" || type == "MSE") { // alias allowed if you like
    ensure_no_options();

  } else if (type == "L1Loss") {
    ensure_no_options();

  } else if (type == "CrossEntropy") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "label_smoothing" });

  } else if (type == "BinaryCrossEntropy" /*|| type == "BCEWithLogits"*/) {
    cuwacunu::camahjucunu::validate_options_exact(row, { "pos_weight" });

  } else if (type == "SmoothL1") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "beta" });

  } else if (type == "Hinge") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "margin" });

  } else if (type == "VICReg") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "sim_coeff", "std_coeff", "cov_coeff", "huber_delta" });

  } else {
    throw std::runtime_error("Unknown loss_function type: " + type);
  }
}

// /* A light-weight "view" of model outputs.
//  * Fill only the slots your model produces and set the bitmask accordingly.
//  */
// struct OutView {
//   // Optional slots (filled by each model as needed)
//   const at::Tensor* logits = nullptr;  // classifiers: raw logits
//   const at::Tensor* pred   = nullptr;  // regressors: predicted values/scores
//   const at::Tensor* log_pi = nullptr;  // MDN: mixture log-weights [B,K]
//   const at::Tensor* mu     = nullptr;  // MDN: means [B,K,D]
//   const at::Tensor* sigma  = nullptr;  // MDN: stddevs [B,K,D]

//   enum Bits : uint32_t { Logits=1u<<0, Pred=1u<<1, LogPi=1u<<2, Mu=1u<<3, Sigma=1u<<4 };
//   uint32_t mask = 0;

//   inline bool has(uint32_t b) const { return (mask & b) != 0; }

//   // Convenience builders
//   static inline OutView from_mdn(const at::Tensor& log_pi,
//                                  const at::Tensor& mu,
//                                  const at::Tensor& sigma) {
//     OutView v; v.log_pi=&log_pi; v.mu=&mu; v.sigma=&sigma; v.mask=LogPi|Mu|Sigma; return v;
//   }
//   static inline OutView from_pred(const at::Tensor& pred_) {
//     OutView v; v.pred=&pred_; v.mask=Pred; return v;
//   }
//   static inline OutView from_logits(const at::Tensor& logits_) {
//     OutView v; v.logits=&logits_; v.mask=Logits; return v;
//   }
// };

// /* Loss interface: implement operator() to compute a scalar */
// struct ILoss {
//   virtual ~ILoss() = default;

//   // 1) OutView entry. Default: forward to tensor overload if `pred` exists.
//   virtual at::Tensor operator()(const OutView& out,
//                                 const at::Tensor& target) const {
//     // Adjust these to your OutView API
//     TORCH_CHECK(out.has(OutView::Pred),
//       "ILoss(OutView,...): no 'pred' in OutView; either override this "
//       "overload or provide a tensor-based overload and include 'pred'.");
//     return (*this)(*out.pred, target);  // calls the tensor overload (virtual)
//   }

//   // 2) Tensor entry. Default: complain unless a derived class provides it.
//   virtual at::Tensor operator()(const at::Tensor& pred,
//                                 const at::Tensor& target) const {
//     TORCH_CHECK(false,
//       "ILoss(Tensor,Tensor): not implemented. Override in derived class.");
//   }
// };

// /* Standard regression losses */
// struct MSELoss final : ILoss {
//   at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
//     TORCH_CHECK(out.has(OutView::Pred), "MSELoss expects pred");
//     return torch::mse_loss(*out.pred, target, at::Reduction::Mean);
//   }
// };

// struct L1Loss final : ILoss {
//   at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
//     TORCH_CHECK(out.has(OutView::Pred), "L1Loss expects pred");
//     return torch::l1_loss(*out.pred, target, at::Reduction::Mean);
//   }
// };

// /* Classification losses */
// struct CrossEntropyLoss final : ILoss {
//   explicit CrossEntropyLoss(double label_smoothing=0.0)
//   : label_smoothing_(label_smoothing) {}

//   at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
//     TORCH_CHECK(out.has(OutView::Logits), "CrossEntropyLoss expects logits");
//     // target is class indices (Long) or prob dists depending on F.cross_entropy semantics
//     using namespace torch::nn::functional;
//     CrossEntropyFuncOptions opts;
//     if (label_smoothing_ != 0.0) {
//       opts = opts.label_smoothing(label_smoothing_);
//     }
//     return cross_entropy(*out.logits, target, opts);
//   }
// private:
//   double label_smoothing_;
// };

// struct BCEWithLogitsLoss final : ILoss {
//   explicit BCEWithLogitsLoss(double pos_weight = 1.0) : pos_weight_(pos_weight) {}

//   at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
//     assert(out.has(OutView::Logits));
//     namespace F = torch::nn::functional;
//     F::BinaryCrossEntropyWithLogitsFuncOptions opts;
//     // pos_weight must be a Tensor (broadcastable to logits). Scalar is fine.
//     auto pw = torch::full({}, pos_weight_, out.logits->options());
//     opts = opts.pos_weight(pw);
//     // (optional) opts = opts.reduction(torch::kMean); // default is Mean already
//     // Ensure target is floating (BCE expects float targets)
//     auto tgt = target.dtype() == out.logits->dtype()
//                  ? target
//                  : target.to(out.logits->dtype());

//     return F::binary_cross_entropy_with_logits(*out.logits, tgt, opts);
//   }

// private:
//   double pos_weight_;
// };
// /* Robust regression (Huber) */
// struct SmoothL1Loss final : ILoss {
//   explicit SmoothL1Loss(double beta=1.0) : beta_(beta) {}

//   at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
//     TORCH_CHECK(out.has(OutView::Pred), "SmoothL1Loss expects pred");
//     using namespace torch::nn::functional;
//     SmoothL1LossFuncOptions opts;
//     opts = opts.beta(beta_);
//     return smooth_l1_loss(*out.pred, target, opts);
//   }
// private:
//   double beta_;
// };

// /* Binary hinge embedding loss (targets in {-1, +1}) */
// struct HingeEmbeddingLoss final : ILoss {
//   explicit HingeEmbeddingLoss(double margin=1.0) : margin_(margin) {}

//   at::Tensor operator()(const OutView& out, const at::Tensor& target) const override {
//     TORCH_CHECK(out.has(OutView::Pred), "HingeEmbeddingLoss expects pred (scores)");
//     using namespace torch::nn::functional;
//     HingeEmbeddingLossFuncOptions opts;
//     opts = opts.margin(margin_);
//     return hinge_embedding_loss(*out.pred, target, opts);
//   }
// private:
//   double margin_;
// };
// /* -------------------------- Row -> Builder ----------------------------- */

// /* Map a config-row (BNF) to a concrete loss.
//  * - Enforces exact columns: {row_id, type, options}
//  * - Enforces exact options per loss (no extras, no missing)
//  */
// inline std::unique_ptr<ILoss>
// make_loss_from_row(const std::unordered_map<std::string,std::string>& row) {
  // Schema: exact columns, but allow options to be "-" for losses with no params.
//   cuwacunu::camahjucunu::require_columns_exact(row, { ROW_ID_COLUMN_HEADER, "type", "options" }, /*enforce_nonempty=*/false);

//   const std::string type = cuwacunu::camahjucunu::require_column(row, "type");

//   // Helper: assert that options is empty/"-" (i.e., no key=value pairs)
//   auto ensure_no_options = [&]() {
//     auto it = row.find("options");
//     if (it == row.end()) return;
//     auto kv = cuwacunu::camahjucunu::parse_options_kvlist(it->second);
//     if (!kv.empty()) {
//       std::ostringstream oss;
//       oss << "Unexpected options for loss_function type='" << type << "'. None are allowed.";
//       throw std::runtime_error(oss.str());
//     }
//   };

//   if (type == "NLLLoss") {         // MDN-NLL (no configurable options)
//     ensure_no_options();
//     return std::make_unique<MdnNllLoss>();
//   }

//   if (type == "MeanSquaredError") { // MSE (no options)
//     ensure_no_options();
//     return std::make_unique<MSELoss>();
//   }

//   if (type == "L1Loss") {           // L1 (no options)
//     ensure_no_options();
//     return std::make_unique<L1Loss>();
//   }

//   if (type == "CrossEntropy") {
//     cuwacunu::camahjucunu::validate_options_exact(row, { "label_smoothing" });
//     const double ls = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "label_smoothing"));
//     return std::make_unique<CrossEntropyLoss>(ls);
//   }

//   if (type == "BinaryCrossEntropy") { // BCEWithLogits variant
//     cuwacunu::camahjucunu::validate_options_exact(row, { "pos_weight" });
//     const double pw = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "pos_weight"));
//     return std::make_unique<BCEWithLogitsLoss>(pw);
//   }

//   if (type == "SmoothL1") {
//     cuwacunu::camahjucunu::validate_options_exact(row, { "beta" });
//     const double beta = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "beta"));
//     return std::make_unique<SmoothL1Loss>(beta);
//   }

//   if (type == "Hinge") {
//     cuwacunu::camahjucunu::validate_options_exact(row, { "margin" });
//     const double margin = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "margin"));
//     return std::make_unique<HingeEmbeddingLoss>(margin);
//   }

//   if (type == "VICReg") {
//     cuwacunu::camahjucunu::validate_options_exact(row, { "sim_coeff", "std_coeff", "cov_coeff" });
//     const double simc = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "sim_coeff"));
//     const double stdc = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "std_coeff"));
//     const double covc = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "cov_coeff"));
//     return std::make_unique<VicRegLoss>(simc, stdc, covc);
//   }

//   throw std::runtime_error("Unknown loss_function type: " + type);
// }

// inline std::unique_ptr<ILoss> make_loss(const cuwacunu::camahjucunu::training_instruction_t& inst, const std::string& row_id) {
//   const auto& row = inst.retrive_row("loss_functions_table", row_id);
//   return make_loss_from_row(row);
// }

} // namespace jkimyei
} // namespace cuwacunu
