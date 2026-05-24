// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "kikijyeba/marshal/dispatch_receipt.h"

namespace cuwacunu::kikijyeba::marshal {

struct marshal_performance_stage_t {
  std::string name{};
  std::string layer{};
  double elapsed_ms{0.0};
  double budget_ms{0.0};
  std::size_t sample_count{1};
  double min_ms{0.0};
  double max_ms{0.0};
  double p50_ms{0.0};
  double p95_ms{0.0};
  bool passed{false};
};

struct marshal_performance_report_t {
  std::vector<marshal_performance_stage_t> stages{};
  std::string non_authority_statement{
      "performance benchmark speed is not target satisfaction evidence"};

  [[nodiscard]] bool passed() const {
    for (const auto &stage : stages) {
      if (!stage.passed) {
        return false;
      }
    }
    return true;
  }
};

template <typename Fn>
[[nodiscard]] inline marshal_performance_stage_t
measure_stage(const std::string &name, const std::string &layer,
              double budget_ms, Fn &&fn) {
  const auto begin = std::chrono::steady_clock::now();
  std::forward<Fn>(fn)();
  const auto end = std::chrono::steady_clock::now();
  const double elapsed_ms =
      std::chrono::duration<double, std::milli>(end - begin).count();
  return marshal_performance_stage_t{.name = name,
                                     .layer = layer,
                                     .elapsed_ms = elapsed_ms,
                                     .budget_ms = budget_ms,
                                     .sample_count = 1,
                                     .min_ms = elapsed_ms,
                                     .max_ms = elapsed_ms,
                                     .p50_ms = elapsed_ms,
                                     .p95_ms = elapsed_ms,
                                     .passed = elapsed_ms <= budget_ms};
}

template <typename Fn>
[[nodiscard]] inline marshal_performance_stage_t
measure_stage_repeated(const std::string &name, const std::string &layer,
                       double budget_ms, std::size_t sample_count, Fn &&fn) {
  if (sample_count == 0U) {
    sample_count = 1U;
  }
  std::vector<double> samples;
  samples.reserve(sample_count);
  for (std::size_t i = 0; i < sample_count; ++i) {
    const auto stage = measure_stage(name, layer, budget_ms, fn);
    samples.push_back(stage.elapsed_ms);
  }
  std::sort(samples.begin(), samples.end());
  double sum = 0.0;
  for (const auto sample : samples) {
    sum += sample;
  }
  const auto percentile_at = [&](double percentile) {
    const double scaled = percentile * static_cast<double>(samples.size() - 1U);
    return samples[static_cast<std::size_t>(scaled + 0.5)];
  };
  const double mean = sum / static_cast<double>(samples.size());
  const double p95 = percentile_at(0.95);
  return marshal_performance_stage_t{.name = name,
                                     .layer = layer,
                                     .elapsed_ms = mean,
                                     .budget_ms = budget_ms,
                                     .sample_count = samples.size(),
                                     .min_ms = samples.front(),
                                     .max_ms = samples.back(),
                                     .p50_ms = percentile_at(0.50),
                                     .p95_ms = p95,
                                     .passed = p95 <= budget_ms};
}

inline void add_stage(marshal_performance_report_t *report,
                      marshal_performance_stage_t stage) {
  if (report) {
    report->stages.push_back(std::move(stage));
  }
}

[[nodiscard]] inline std::string
canonical_performance_report_text(const marshal_performance_report_t &report) {
  std::ostringstream out;
  detail::append_kv(out, "non_authority_statement",
                    report.non_authority_statement);
  detail::append_kv(out, "stage_count", std::to_string(report.stages.size()));
  for (std::size_t i = 0; i < report.stages.size(); ++i) {
    const auto &stage = report.stages[i];
    const auto prefix = "stage." + std::to_string(i);
    detail::append_kv(out, prefix + ".name", stage.name);
    detail::append_kv(out, prefix + ".layer", stage.layer);
    detail::append_kv(out, prefix + ".elapsed_ms",
                      std::to_string(stage.elapsed_ms));
    detail::append_kv(out, prefix + ".budget_ms",
                      std::to_string(stage.budget_ms));
    detail::append_kv(out, prefix + ".sample_count",
                      std::to_string(stage.sample_count));
    detail::append_kv(out, prefix + ".min_ms", std::to_string(stage.min_ms));
    detail::append_kv(out, prefix + ".max_ms", std::to_string(stage.max_ms));
    detail::append_kv(out, prefix + ".p50_ms", std::to_string(stage.p50_ms));
    detail::append_kv(out, prefix + ".p95_ms", std::to_string(stage.p95_ms));
    detail::append_kv(out, prefix + ".passed", detail::bool_text(stage.passed));
  }
  return out.str();
}

[[nodiscard]] inline std::string
performance_budget_failure_hint(const marshal_performance_report_t &report) {
  for (const auto &stage : report.stages) {
    if (!stage.passed) {
      return "Marshal performance budget exceeded at " + stage.layer + "/" +
             stage.name;
    }
  }
  return {};
}

} // namespace cuwacunu::kikijyeba::marshal
