/* jk_lr_schedulers.h
 *
 * Builders for LibTorch learning-rate schedulers, wired from a BNF row.
 * This version avoids torch/optim/lr_scheduler.h and implements tiny fallbacks.
 *
 * Supported (tiny): StepLR, MultiStepLR, ExponentialLR
 * Not supported here: ReduceLROnPlateau, OneCycleLR (throws)
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <torch/torch.h>
#include <torch/optim/optimizer.h>

#include "camahjucunu/BNF/implementations/training_components/training_components.h"

namespace cuwacunu {
namespace jkimyei {

/* ----------------------- Minimal scheduler base ----------------------- */
struct LRSchedulerAny {
  virtual ~LRSchedulerAny() = default;
  virtual void step() {}
  virtual void step(double /*metric*/) { step(); }
};

/* -------------------------- Builder interface ------------------------- */
struct ISchedulerBuilder {
  virtual ~ISchedulerBuilder() = default;
  virtual std::unique_ptr<LRSchedulerAny>
  build(torch::optim::Optimizer& opt) const = 0;
};

/* -------- Helper: scale LR across optimizer param groups -------------- */
inline void scale_optimizer_lr(torch::optim::Optimizer& opt, double factor) {
  for (auto& pg : opt.param_groups()) {
    const double lr = pg.options().get_lr();
    pg.options().set_lr(lr * factor);
  }
}

/* ============================== StepLR ================================= */

struct StepLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  int64_t step_size;
  double  gamma;
  int64_t epoch{0};

  StepLRTiny(torch::optim::Optimizer& o, int64_t s, double g)
      : opt(&o), step_size(s), gamma(g) {}

  void step() override {
    ++epoch;
    if (step_size > 0 && (epoch % step_size) == 0) {
      scale_optimizer_lr(*opt, gamma);
    }
  }
};

struct StepLRBuilder final : ISchedulerBuilder {
  int64_t step_size; double gamma;
  StepLRBuilder(int64_t s, double g) : step_size(s), gamma(g) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<StepLRTiny>(opt, step_size, gamma);
  }
};

/* ============================ MultiStepLR ============================== */

struct MultiStepLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  std::vector<int64_t> milestones;
  double gamma;
  int64_t epoch{0};

  MultiStepLRTiny(torch::optim::Optimizer& o, std::vector<int64_t> ms, double g)
      : opt(&o), milestones(std::move(ms)), gamma(g) {
    std::sort(milestones.begin(), milestones.end());
    milestones.erase(std::unique(milestones.begin(), milestones.end()), milestones.end());
  }

  void step() override {
    ++epoch;
    if (std::binary_search(milestones.begin(), milestones.end(), epoch)) {
      scale_optimizer_lr(*opt, gamma);
    }
  }
};

struct MultiStepLRBuilder final : ISchedulerBuilder {
  std::vector<int64_t> milestones; double gamma;
  MultiStepLRBuilder(std::vector<int64_t> ms, double g)
      : milestones(std::move(ms)), gamma(g) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<MultiStepLRTiny>(opt, milestones, gamma);
  }
};

/* =========================== ExponentialLR ============================= */

struct ExponentialLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  double gamma;

  ExponentialLRTiny(torch::optim::Optimizer& o, double g)
      : opt(&o), gamma(g) {}

  void step() override {
    scale_optimizer_lr(*opt, gamma);
  }
};

struct ExponentialLRBuilder final : ISchedulerBuilder {
  double gamma;
  explicit ExponentialLRBuilder(double g) : gamma(g) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<ExponentialLRTiny>(opt, gamma);
  }
};

/* ======================= ReduceLROnPlateau (stub) ===================== */

struct ReduceLROnPlateauBuilder final : ISchedulerBuilder {
  std::string mode; double factor; int64_t patience; double threshold;
  std::string threshold_mode; int64_t cooldown; double min_lr; double eps;

  ReduceLROnPlateauBuilder(std::string m, double f, int64_t p, double th,
                           std::string thm, int64_t cd, double minlr, double e)
      : mode(std::move(m)), factor(f), patience(p), threshold(th),
        threshold_mode(std::move(thm)), cooldown(cd), min_lr(minlr), eps(e) {}

  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& /*opt*/) const override {
    throw std::runtime_error(
      "ReduceLROnPlateau not available in this build; remove from config or upgrade LibTorch.");
  }
};

/* ============================ OneCycleLR (stub) ======================== */

struct OneCycleLRBuilder final : ISchedulerBuilder {
  double  max_lr;
  int64_t total_steps;
  OneCycleLRBuilder(double max_lr_, int64_t total_steps_)
      : max_lr(max_lr_), total_steps(total_steps_) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& /*opt*/) const override {
    throw std::runtime_error(
      "OneCycleLR not available in this build; remove from config or upgrade LibTorch.");
  }
};

/* ---------------------- Row -> Builder mapping ------------------------ */

inline std::unique_ptr<ISchedulerBuilder>
make_scheduler_builder_from_row(const std::unordered_map<std::string, std::string>& row) {

  // Exact columns for this table
  cuwacunu::camahjucunu::require_columns_exact(row, { ROW_ID_COLUMN_HEADER, "scheduler_type", "options" });

  const std::string type = cuwacunu::camahjucunu::require_column(row, "scheduler_type");

  if (type == "StepLR") {
    // Required, no extras
    cuwacunu::camahjucunu::validate_options_exact(row, { "step_size", "gamma" });

    const int64_t step_size = static_cast<int64_t>(cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_option(row, "step_size")));
    const double  gamma     = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "gamma"));
    return std::make_unique<StepLRBuilder>(step_size, gamma);
  }

  if (type == "MultiStepLR") {
    // milestones OR step_size (CSV list), plus gamma; no extras
    cuwacunu::camahjucunu::validate_options_exact(row, { "milestones|step_size", "gamma" });

    const std::string csv = cuwacunu::camahjucunu::require_any_option(row, { "milestones", "step_size" });
    const auto ms_long    = cuwacunu::camahjucunu::to_long_list_csv(csv);  // e.g. "10,20,40"
    std::vector<int64_t> milestones(ms_long.begin(), ms_long.end());

    const double gamma = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "gamma"));
    return std::make_unique<MultiStepLRBuilder>(std::move(milestones), gamma);
  }

  if (type == "ExponentialLR") {
    // Required, no extras
    cuwacunu::camahjucunu::validate_options_exact(row, { "gamma" });

    const double gamma = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "gamma"));
    return std::make_unique<ExponentialLRBuilder>(gamma);
  }

  if (type == "ReduceLROnPlateau") {
    // Fully specified, no extras
    cuwacunu::camahjucunu::validate_options_exact(row, {
      "mode",
      "factor",
      "patience",
      "threshold",
      "threshold_mode",
      "cooldown",
      "min_lr",
      "eps"
    });

    return std::make_unique<ReduceLROnPlateauBuilder>(
      cuwacunu::camahjucunu::require_option(row, "mode"),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "factor")),
      static_cast<int64_t>(cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_option(row, "patience"))),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "threshold")),
      cuwacunu::camahjucunu::require_option(row, "threshold_mode"),
      static_cast<int64_t>(cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_option(row, "cooldown"))),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "min_lr")),
      cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_option(row, "eps"))
    );
  }

  if (type == "OneCycleLR") {
    // Stub: accept either {max_lr,total_steps} or legacy {gamma,step_size}; no extras
    cuwacunu::camahjucunu::validate_options_exact(row, { "max_lr|gamma", "total_steps|step_size" });

    const double  max_lr      = cuwacunu::camahjucunu::to_double(cuwacunu::camahjucunu::require_any_option(row, { "max_lr", "gamma" }));
    const int64_t total_steps = static_cast<int64_t>(cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_any_option(row, { "total_steps", "step_size" })));

    return std::make_unique<OneCycleLRBuilder>(max_lr, total_steps);
  }

  throw std::runtime_error("Unknown scheduler_type: " + type);
}

/* Convenience: fetch row from instruction and return a builder. */
inline std::unique_ptr<ISchedulerBuilder>
make_scheduler_builder(const cuwacunu::camahjucunu::BNF::training_instruction_t& inst,
                       const std::string& row_id) {
  const auto& row = inst.retrive_row("lr_schedulers_table", row_id);
  return make_scheduler_builder_from_row(row);
}

} // namespace jkimyei
} // namespace cuwacunu
