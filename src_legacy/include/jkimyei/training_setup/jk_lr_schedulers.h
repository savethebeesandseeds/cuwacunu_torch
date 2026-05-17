/* jk_lr_schedulers.h
 *
 * Builders for LibTorch learning-rate schedulers, wired from a DSL spec row.
 * This version avoids torch/optim/lr_scheduler.h and implements tiny fallbacks.
 *
 * Supported (tiny):
 *   - ConstantLR                  (PerEpoch)
 *   - StepLR                      (PerEpoch)
 *   - MultiStepLR                 (PerEpoch)
 *   - ExponentialLR               (PerEpoch)
 *   - ReduceLROnPlateau           (PerEpochWithMetric)
 *   - OneCycleLR (triangular LR)  (PerBatch)
 *   - CosineAnnealingLR           (PerEpoch)
 *   - WarmupLR (linear, then hold)(PerBatch)
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>
#include <torch/optim/optimizer.h>

#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "jkimyei/api/schema_catalog.h"

namespace cuwacunu {
namespace jkimyei {

inline void ensure_scheduler_builder_coverage() {
  static const bool kCoverageChecked = [] {
    const std::unordered_set<std::string> kImplemented = {
        "StepLR",
        "MultiStepLR",
        "OneCycleLR",
        "ExponentialLR",
        "ReduceLROnPlateau",
        "ConstantLR",
        "CosineAnnealingLR",
        "WarmupLR"};
    for (const auto& type : cuwacunu::jkimyei::api::supported_scheduler_types()) {
      if (kImplemented.find(type) == kImplemented.end()) {
        throw std::runtime_error(
            "scheduler declared in jkimyei_schema.def but not implemented in jk_lr_schedulers: " +
            type);
      }
    }
    return true;
  }();
  (void)kCoverageChecked;
}

/* ----------------------- Minimal scheduler base ----------------------- */
struct LRSchedulerAny {
  enum class Mode { PerBatch, PerEpoch, PerEpochWithMetric };
  Mode mode{Mode::PerEpoch};                 // default = per-epoch stepping

  virtual ~LRSchedulerAny() = default;
  virtual void step() {}                     // no-metric step
  virtual void step(double metric) {         // default: ignore metric
    (void)metric; step();
  }
};

/* -------------------------- Builder interface ------------------------- */
struct ISchedulerBuilder {
  virtual ~ISchedulerBuilder() = default;
  virtual std::unique_ptr<LRSchedulerAny>
  build(torch::optim::Optimizer& opt) const = 0;
};

/* -------- Helper: LR utils ------------------------------------------- */
inline void scale_optimizer_lr(torch::optim::Optimizer& opt, double factor) {
  for (auto& pg : opt.param_groups()) {
    const double lr = pg.options().get_lr();
    pg.options().set_lr(lr * factor);
  }
}
inline void set_optimizer_lrs(torch::optim::Optimizer& opt,
                              const std::vector<double>& lrs) {
  auto& groups = opt.param_groups();
  if (lrs.size() != groups.size())
    throw std::runtime_error("set_optimizer_lrs: size mismatch");
  for (std::size_t i = 0; i < groups.size(); ++i)
    groups[i].options().set_lr(lrs[i]);
}
inline std::vector<double> get_optimizer_lrs(const torch::optim::Optimizer& opt) {
  std::vector<double> out;
  out.reserve(opt.param_groups().size());
  for (auto const& pg : opt.param_groups())
    out.push_back(pg.options().get_lr());
  return out;
}

/* ============================== ConstantLR ============================ */
/* Holds LR constant at the optimizer's current LR (or an absolute LR if provided). */
struct ConstantLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  std::vector<double> fixed_lrs;

  ConstantLRTiny(torch::optim::Optimizer& o, double absolute_lr /*<=0 means keep current*/)
      : opt(&o) {
    mode = Mode::PerEpoch;
    if (absolute_lr > 0.0) {
      fixed_lrs.assign(o.param_groups().size(), absolute_lr);
      set_optimizer_lrs(*opt, fixed_lrs);
    } else {
      fixed_lrs = get_optimizer_lrs(o);
    }
  }
  void step() override { set_optimizer_lrs(*opt, fixed_lrs); }
  void step(double /*metric*/) override { step(); }
};

struct ConstantLRBuilder final : ISchedulerBuilder {
  double absolute_lr; // <=0 -> keep current
  explicit ConstantLRBuilder(double lr_abs) : absolute_lr(lr_abs) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<ConstantLRTiny>(opt, absolute_lr);
  }
};

/* ============================== StepLR ================================ */
struct StepLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  int64_t step_size;
  double  gamma;
  int64_t epoch{0};

  StepLRTiny(torch::optim::Optimizer& o, int64_t s, double g)
      : opt(&o), step_size(s), gamma(g) {
    mode = Mode::PerEpoch;
  }

  void step() override {
    ++epoch;
    if (step_size > 0 && (epoch % step_size) == 0)
      scale_optimizer_lr(*opt, gamma);
  }
  void step(double /*metric*/) override { step(); }
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
    mode = Mode::PerEpoch;
    std::sort(milestones.begin(), milestones.end());
    milestones.erase(std::unique(milestones.begin(), milestones.end()), milestones.end());
  }

  void step() override {
    ++epoch;
    if (std::binary_search(milestones.begin(), milestones.end(), epoch))
      scale_optimizer_lr(*opt, gamma);
  }
  void step(double /*metric*/) override { step(); }
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
      : opt(&o), gamma(g) {
    mode = Mode::PerEpoch;
  }

  void step() override { scale_optimizer_lr(*opt, gamma); }
  void step(double /*metric*/) override { step(); }
};

struct ExponentialLRBuilder final : ISchedulerBuilder {
  double gamma;
  explicit ExponentialLRBuilder(double g) : gamma(g) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<ExponentialLRTiny>(opt, gamma);
  }
};

/* ======================= ReduceLROnPlateau (tiny) ===================== */
struct ReduceLROnPlateauTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;

  enum class ModeCmp { Min, Max };
  enum class ThMode { Rel, Abs };

  ModeCmp  mode_cmp;
  double   factor;        // (0,1)
  int64_t  patience;      // >=0
  double   threshold;     // >=0
  ThMode   threshold_mode;
  int64_t  cooldown;      // >=0
  double   min_lr;        // >=0
  double   eps;           // >=0

  double   best;
  bool     has_best{false};
  int64_t  num_bad_epochs{0};
  int64_t  cooldown_counter{0};

  explicit ReduceLROnPlateauTiny(
      torch::optim::Optimizer& o,
      std::string m,
      double f,
      int64_t p,
      double th,
      std::string thm,
      int64_t cd,
      double minlr,
      double e)
      : opt(&o),
        mode_cmp((m == "max" || m == "MAX") ? ModeCmp::Max : ModeCmp::Min),
        factor(f),
        patience(std::max<int64_t>(0, p)),
        threshold(std::max(0.0, th)),
        threshold_mode((thm == "abs" || thm == "ABS" || thm == "absolute") ? ThMode::Abs : ThMode::Rel),
        cooldown(std::max<int64_t>(0, cd)),
        min_lr(std::max(0.0, minlr)),
        eps(std::max(0.0, e)),
        best( (mode_cmp == ModeCmp::Min) ? std::numeric_limits<double>::infinity()
                                         : -std::numeric_limits<double>::infinity() ) {
    mode = Mode::PerEpochWithMetric;
    if (!(factor > 0.0 && factor < 1.0))
      throw std::runtime_error("ReduceLROnPlateau: factor must be in (0,1).");
  }

  bool is_better(double a, double b) const {
    if (std::isnan(a) || std::isnan(b)) return false;
    if (mode_cmp == ModeCmp::Min) {
      return (threshold_mode == ThMode::Rel) ? (a < b * (1.0 - threshold))
                                             : (a < b - threshold);
    } else {
      return (threshold_mode == ThMode::Rel) ? (a > b * (1.0 + threshold))
                                             : (a > b + threshold);
    }
  }

  void maybe_reduce() {
    if (cooldown_counter > 0) { --cooldown_counter; return; }
    if (num_bad_epochs <= patience) return;

    auto lrs = get_optimizer_lrs(*opt);
    bool changed = false;
    for (double& lr : lrs) {
      const double new_lr = std::max(min_lr, lr * factor);
      if ((lr - new_lr) > eps) { lr = new_lr; changed = true; }
    }
    if (changed) {
      set_optimizer_lrs(*opt, lrs);
      num_bad_epochs = 0;
      cooldown_counter = cooldown;
    }
  }

  void step(double metric) override {
    if (!std::isnan(metric)) {
      if (!has_best) { best = metric; has_best = true; num_bad_epochs = 0; }
      else if (is_better(metric, best)) { best = metric; num_bad_epochs = 0; }
      else { ++num_bad_epochs; }
    }
    maybe_reduce();
  }
  void step() override { /* no-op without metric */ }
};

struct ReduceLROnPlateauBuilder final : ISchedulerBuilder {
  std::string mode; double factor; int64_t patience; double threshold;
  std::string threshold_mode; int64_t cooldown; double min_lr; double eps;

  ReduceLROnPlateauBuilder(std::string m, double f, int64_t p, double th,
                           std::string thm, int64_t cd, double minlr, double e)
      : mode(std::move(m)), factor(f), patience(p), threshold(th),
        threshold_mode(std::move(thm)), cooldown(cd), min_lr(minlr), eps(e) {}

  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<ReduceLROnPlateauTiny>(
      opt, mode, factor, patience, threshold, threshold_mode, cooldown, min_lr, eps
    );
  }
};

/* ============================ OneCycleLR (tiny) ======================== */
/* Accepts either absolute max_lr OR multiplicative mult, plus total_steps. */
struct OneCycleLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  std::vector<double> base_lrs;
  std::vector<double> peak_lrs;
  int64_t total_steps;
  int64_t step_count{0};
  static constexpr double pct_start = 0.30;

  OneCycleLRTiny(torch::optim::Optimizer& o,
                 double value, bool use_abs, int64_t total_steps_)
      : opt(&o),
        base_lrs(get_optimizer_lrs(o)),
        total_steps(std::max<int64_t>(1, total_steps_)) {
    mode = Mode::PerBatch;
    peak_lrs.reserve(base_lrs.size());
    for (double b : base_lrs) {
      double peak = use_abs ? value : (b * value);
      if (peak < b) std::swap(peak, b);           // ensure ramp up then down
      peak_lrs.push_back(peak);
    }
  }

  void step() override {
    if (step_count >= total_steps) { set_optimizer_lrs(*opt, base_lrs); return; }
    const int64_t up = std::max<int64_t>(1, static_cast<int64_t>(std::llround(pct_start * total_steps)));
    const int64_t dn = std::max<int64_t>(1, total_steps - up);

    std::vector<double> new_lrs(base_lrs.size());
    if (step_count < up) {
      const double t = double(step_count + 1) / double(up);
      for (std::size_t i = 0; i < new_lrs.size(); ++i)
        new_lrs[i] = base_lrs[i] + (peak_lrs[i] - base_lrs[i]) * t;
    } else {
      const int64_t k = step_count - up;
      const double t = double(k + 1) / double(dn);
      for (std::size_t i = 0; i < new_lrs.size(); ++i)
        new_lrs[i] = peak_lrs[i] - (peak_lrs[i] - base_lrs[i]) * t;
    }
    set_optimizer_lrs(*opt, new_lrs);
    ++step_count;
  }
  void step(double /*metric*/) override { step(); }
};

struct OneCycleLRBuilder final : ISchedulerBuilder {
  double value; bool use_abs; int64_t total_steps;
  OneCycleLRBuilder(double v, bool abs_mode, int64_t ts)
      : value(v), use_abs(abs_mode), total_steps(ts) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<OneCycleLRTiny>(opt, value, use_abs, total_steps);
  }
};

/* ========================= CosineAnnealingLR (tiny) =================== */
/* Cosine decay from base_lrs to eta_min over T_max steps. */
struct CosineAnnealingLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  std::vector<double> base_lrs;
  double eta_min;
  int64_t T_max;
  int64_t t{0}; // steps taken

  CosineAnnealingLRTiny(torch::optim::Optimizer& o, int64_t T_max_, double eta_min_)
      : opt(&o),
        base_lrs(get_optimizer_lrs(o)),
        eta_min(std::max(0.0, eta_min_)),
        T_max(std::max<int64_t>(1, T_max_)) {
    mode = Mode::PerEpoch;
  }

  void step() override {
    const double ct = std::min<int64_t>(t, T_max);
    const double cos_term = (1.0 + std::cos(M_PI * ct / double(T_max))) * 0.5; // 1..0
    std::vector<double> lrs(base_lrs.size());
    for (std::size_t i = 0; i < lrs.size(); ++i)
      lrs[i] = eta_min + (base_lrs[i] - eta_min) * cos_term;
    set_optimizer_lrs(*opt, lrs);
    if (t < T_max) ++t;
  }
  void step(double /*metric*/) override { step(); }
};

struct CosineAnnealingLRBuilder final : ISchedulerBuilder {
  int64_t T_max; double eta_min;
  CosineAnnealingLRBuilder(int64_t T, double e) : T_max(T), eta_min(e) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<CosineAnnealingLRTiny>(opt, T_max, eta_min);
  }
};

/* ============================== WarmupLR (tiny) ======================= */
/* Linear warmup: from start_factor*base -> end_factor*base over warmup_steps.
 * After warmup, it holds at end_factor*base (no inner scheduler here, by design).
 */
struct WarmupLinearLRTiny final : LRSchedulerAny {
  torch::optim::Optimizer* opt;
  std::vector<double> base_lrs;
  double start_factor;
  double end_factor;
  int64_t warmup_steps;
  int64_t t{0};

  WarmupLinearLRTiny(torch::optim::Optimizer& o,
                     int64_t warm_steps,
                     double start_f,
                     double end_f)
      : opt(&o),
        base_lrs(get_optimizer_lrs(o)),
        start_factor(std::max(0.0, start_f)),
        end_factor(std::max(0.0, end_f)),
        warmup_steps(std::max<int64_t>(1, warm_steps)) {
    mode = Mode::PerBatch;
  }

  void step() override {
    std::vector<double> lrs(base_lrs.size());
    if (t < warmup_steps) {
      const double a = double(t + 1) / double(warmup_steps);
      const double f = start_factor + (end_factor - start_factor) * a;
      for (std::size_t i = 0; i < lrs.size(); ++i) lrs[i] = base_lrs[i] * f;
      ++t;
    } else {
      for (std::size_t i = 0; i < lrs.size(); ++i) lrs[i] = base_lrs[i] * end_factor;
    }
    set_optimizer_lrs(*opt, lrs);
  }
  void step(double /*metric*/) override { step(); }
};

struct WarmupLRBuilder final : ISchedulerBuilder {
  int64_t warmup_steps; double start_factor; double end_factor;
  WarmupLRBuilder(int64_t s, double a, double b)
      : warmup_steps(s), start_factor(a), end_factor(b) {}
  std::unique_ptr<LRSchedulerAny> build(torch::optim::Optimizer& opt) const override {
    return std::make_unique<WarmupLinearLRTiny>(opt, warmup_steps, start_factor, end_factor);
  }
};

/* ---------------------- Row -> Builder mapping ------------------------ */

inline std::unique_ptr<ISchedulerBuilder>
make_scheduler_builder_from_row(const cuwacunu::camahjucunu::jkimyei_specs_t::row_t& row) {
  ensure_scheduler_builder_coverage();

  cuwacunu::camahjucunu::require_columns_exact(row, { ROW_ID_COLUMN_HEADER, "type", "options" });
  const std::string type = cuwacunu::camahjucunu::require_column(row, "type");
  cuwacunu::jkimyei::api::require_scheduler_type_registered(type);

  if (type == "ConstantLR") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "lr" });
    const double lr_abs = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(row, "lr")); // 0 => keep current
    return std::make_unique<ConstantLRBuilder>(lr_abs);
  }

  if (type == "StepLR") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "step_size", "gamma" });
    const int64_t step_size = static_cast<int64_t>(
        cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_option(row, "step_size")));
    const double  gamma     = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(row, "gamma"));
    return std::make_unique<StepLRBuilder>(step_size, gamma);
  }

  if (type == "MultiStepLR") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "milestones|step_size", "gamma" });
    const std::string csv = cuwacunu::camahjucunu::require_any_option(row, { "milestones", "step_size" });
    const auto ms_long    = cuwacunu::camahjucunu::to_long_list_csv(csv);
    std::vector<int64_t> milestones(ms_long.begin(), ms_long.end());
    const double gamma = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(row, "gamma"));
    return std::make_unique<MultiStepLRBuilder>(std::move(milestones), gamma);
  }

  if (type == "ExponentialLR") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "gamma" });
    const double gamma = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(row, "gamma"));
    return std::make_unique<ExponentialLRBuilder>(gamma);
  }

  if (type == "ReduceLROnPlateau") {
    cuwacunu::camahjucunu::validate_options_exact(row, {
      "mode","factor","patience","threshold","threshold_mode","cooldown","min_lr","eps"
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
    // Accept either absolute max_lr or multiplicative mult (choose one) + total_steps
    cuwacunu::camahjucunu::validate_options_exact(row, { "max_lr|mult", "total_steps" });
    const bool has_abs = cuwacunu::camahjucunu::has_option(row, "max_lr");
    const double value = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_any_option(row, { "max_lr", "mult" }));
    const int64_t total_steps = static_cast<int64_t>(
        cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_option(row, "total_steps")));
    return std::make_unique<OneCycleLRBuilder>(value, has_abs, total_steps);
  }

  if (type == "CosineAnnealingLR") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "T_max", "eta_min" });
    const int64_t T_max = static_cast<int64_t>(
        cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_option(row, "T_max")));
    const double eta_min = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(row, "eta_min"));
    return std::make_unique<CosineAnnealingLRBuilder>(T_max, eta_min);
  }

  if (type == "WarmupLR") {
    cuwacunu::camahjucunu::validate_options_exact(row, { "warmup_steps", "start_factor", "end_factor" });
    const int64_t warm = static_cast<int64_t>(
        cuwacunu::camahjucunu::to_long(cuwacunu::camahjucunu::require_option(row, "warmup_steps")));
    const double sf = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(row, "start_factor"));
    const double ef = cuwacunu::camahjucunu::to_double(
        cuwacunu::camahjucunu::require_option(row, "end_factor"));
    return std::make_unique<WarmupLRBuilder>(warm, sf, ef);
  }

  throw std::runtime_error("Unknown scheduler type: " + type);
}

/* Convenience: fetch row from instruction and return a builder. */
inline std::unique_ptr<ISchedulerBuilder>
make_scheduler_builder(const cuwacunu::camahjucunu::jkimyei_specs_t& inst,
                       const std::string& row_id) {
  const auto& row = inst.retrive_row("lr_schedulers_table", row_id);
  return make_scheduler_builder_from_row(row);
}

} // namespace jkimyei
} // namespace cuwacunu
