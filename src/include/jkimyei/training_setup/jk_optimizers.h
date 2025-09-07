/* jk_optimizers.h
 *
 * Builder pattern for LibTorch optimizers driven by a BNF-config row.
 *
 * Design goals
 *  - Decouple optimizer construction from model code.
 *  - Avoid copies of parameter tensors (pass by const-ref).
 *  - Be lenient with config key names (e.g., "eps" OR "epsilon").
 *  - Fail fast with clear error messages when required fields are missing.
 *
 * Table expectations
 *  - A row (unordered_map<string,string>) defines "optimizer_type" and
 *    its hyperparameters.
 *  - Common keys (preferred): 
 *      "initial_learning_rate", "weight_decay", "epsilon" or "eps",
 *      "beta1", "beta2", "amsgrad", "momentum", "nesterov",
 *      "alpha", "rho", "decay" (Adagrad lr_decay), "centered",
 *      "lambd", "t0" (ASGD)
 *  - You can add more keys in your table without changing this header.
 */

#pragma once

#include <torch/torch.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <utility>    // std::make_pair

#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace jkimyei {

/* Interface: turns a parameter list into a concrete torch::optim::Optimizer.
 * NOTE: We accept params by const-ref to avoid copying the vector.
 *       The optimizer will internally keep references to the Tensors.
 */
struct IOptimizerBuilder {
  virtual ~IOptimizerBuilder() = default;
  virtual std::unique_ptr<torch::optim::Optimizer>
  build(const std::vector<at::Tensor>& params) const = 0;
};

/* ---------------------- Concrete OptimizerBuilders --------------------- */

struct AdamBuilder final : IOptimizerBuilder {
  double lr, weight_decay, eps, beta1, beta2; bool amsgrad;
  AdamBuilder(double lr_, double wd_, double eps_, double b1_, double b2_, bool am_)
    : lr(lr_), weight_decay(wd_), eps(eps_), beta1(b1_), beta2(b2_), amsgrad(am_) {}
  std::unique_ptr<torch::optim::Optimizer>
  build(const std::vector<at::Tensor>& params) const override {
    torch::optim::AdamOptions opts(lr);
    opts.weight_decay(weight_decay);
    opts.eps(eps);
    opts.betas(std::make_tuple(beta1, beta2));
    opts.amsgrad(amsgrad);
    return std::make_unique<torch::optim::Adam>(params, opts);
  }
};

struct AdamWBuilder final : IOptimizerBuilder {
  double lr, weight_decay, eps, beta1, beta2; bool amsgrad;
  AdamWBuilder(double lr_, double wd_, double eps_, double b1_, double b2_, bool am_)
    : lr(lr_), weight_decay(wd_), eps(eps_), beta1(b1_), beta2(b2_), amsgrad(am_) {}
  std::unique_ptr<torch::optim::Optimizer>
  build(const std::vector<at::Tensor>& params) const override {
    torch::optim::AdamWOptions opts(lr);
    opts.weight_decay(weight_decay);
    opts.eps(eps);
    opts.betas(std::make_tuple(beta1, beta2));
    opts.amsgrad(amsgrad);
    return std::make_unique<torch::optim::AdamW>(params, opts);
  }
};

struct SgdBuilder final : IOptimizerBuilder {
  double lr, momentum, weight_decay; bool nesterov;
  SgdBuilder(double lr_, double mom_, double wd_, bool nes_)
    : lr(lr_), momentum(mom_), weight_decay(wd_), nesterov(nes_) {}
  std::unique_ptr<torch::optim::Optimizer>
  build(const std::vector<at::Tensor>& params) const override {
    torch::optim::SGDOptions opts(lr);
    opts.momentum(momentum);
    opts.weight_decay(weight_decay);
    opts.nesterov(nesterov);
    return std::make_unique<torch::optim::SGD>(params, opts);
  }
};

struct RmspropBuilder final : IOptimizerBuilder {
  double lr, alpha, eps, weight_decay; bool centered;
  RmspropBuilder(double lr_, double alpha_, double eps_, double wd_, bool cen_)
    : lr(lr_), alpha(alpha_), eps(eps_), weight_decay(wd_), centered(cen_) {}
  std::unique_ptr<torch::optim::Optimizer>
  build(const std::vector<at::Tensor>& params) const override {
    torch::optim::RMSpropOptions opts(lr);
    opts.alpha(alpha);
    opts.eps(eps);
    opts.weight_decay(weight_decay);
    opts.centered(centered);
    return std::make_unique<torch::optim::RMSprop>(params, opts);
  }
};

struct AdagradBuilder final : IOptimizerBuilder {
  double lr, lr_decay, eps, weight_decay;
  AdagradBuilder(double lr_, double lrdec_, double eps_, double wd_)
    : lr(lr_), lr_decay(lrdec_), eps(eps_), weight_decay(wd_) {}
  std::unique_ptr<torch::optim::Optimizer>
  build(const std::vector<at::Tensor>& params) const override {
    torch::optim::AdagradOptions opts(lr);
    opts.lr_decay(lr_decay);
    opts.eps(eps);
    opts.weight_decay(weight_decay);
    return std::make_unique<torch::optim::Adagrad>(params, opts);
  }
};


/* -------------------------- Row -> Builder ----------------------------- */

/* Map a config-row (BNF) to a concrete optimizer builder.
 * - Enforces exact columns: {row_id, type, options}
 * - Enforces exact options set per optimizer (no extras, no missing)
 * - Accepts alias "epsilon|eps" where LibTorch expects eps
 */
inline std::unique_ptr<IOptimizerBuilder>
make_optimizer_builder_from_row(const std::unordered_map<std::string,std::string>& row) {
  // Require exact columns for this table
  cuwacunu::camahjucunu::require_columns_exact(row, { ROW_ID_COLUMN_HEADER, "type", "options" });

  const std::string type = cuwacunu::camahjucunu::require_column(row, "type");

  // All optimizers require an initial LR
  const double lr = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "initial_learning_rate"));

  if (type == "Adam") {
    cuwacunu::camahjucunu::validate_options_exact(row, {
      "initial_learning_rate",
      "weight_decay",
      "epsilon|eps",
      "beta1",
      "beta2",
      "amsgrad"
    });
    return std::make_unique<AdamBuilder>(
      lr,
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "weight_decay")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_any_option(row, {"epsilon","eps"})),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "beta1")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "beta2")),
      cuwacunu::camahjucunu::to_bool  (cuwacunu::camahjucunu::require_option(row, "amsgrad"))
    );
  }

  if (type == "AdamW") {
    cuwacunu::camahjucunu::validate_options_exact(row, {
      "initial_learning_rate",
      "weight_decay",
      "epsilon|eps",
      "beta1",
      "beta2",
      "amsgrad"
    });
    return std::make_unique<AdamWBuilder>(
      lr,
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "weight_decay")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_any_option(row, {"epsilon","eps"})),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "beta1")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "beta2")),
      cuwacunu::camahjucunu::to_bool  (cuwacunu::camahjucunu::require_option(row, "amsgrad"))
    );
  }

  if (type == "SGD") {
    cuwacunu::camahjucunu::validate_options_exact(row, {
      "initial_learning_rate",
      "momentum",
      "weight_decay",
      "nesterov"
    });
    return std::make_unique<SgdBuilder>(
      lr,
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "momentum")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "weight_decay")),
      cuwacunu::camahjucunu::to_bool  (cuwacunu::camahjucunu::require_option(row, "nesterov"))
    );
  }

  if (type == "RMSprop") {
    cuwacunu::camahjucunu::validate_options_exact(row, {
      "initial_learning_rate",
      "alpha",
      "epsilon|eps",
      "weight_decay",
      "centered"
    });
    return std::make_unique<RmspropBuilder>(
      lr,
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "alpha")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_any_option(row, {"epsilon","eps"})),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "weight_decay")),
      cuwacunu::camahjucunu::to_bool  (cuwacunu::camahjucunu::require_option(row, "centered"))
    );
  }

  if (type == "Adagrad") {
    cuwacunu::camahjucunu::validate_options_exact(row, {
      "initial_learning_rate",
      "decay",          // interpreted as lr_decay
      "epsilon|eps",
      "weight_decay"
    });
    return std::make_unique<AdagradBuilder>(
      lr,
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "decay")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_any_option(row, {"epsilon","eps"})),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "weight_decay"))
    );
  }

  throw std::runtime_error("Unknown optimizer type: " + type);
}

/* Convenience: pull row from the instruction and delegate. */
inline std::unique_ptr<IOptimizerBuilder>
make_optimizer_builder(const cuwacunu::camahjucunu::training_instruction_t& inst,
                       const std::string& row_id) {
  const auto& row = inst.retrive_row("optimizers_table", row_id);
  return make_optimizer_builder_from_row(row);
}

} // namespace jkimyei
} // namespace cuwacunu
