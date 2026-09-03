#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace cuwacunu::tests::mtf_vicreg_variance_necessity_gate {

constexpr std::size_t kSeedCount = 3;
constexpr std::size_t kFamilyCount = 4;

struct PairedContrast {
  double point{0.0};
  double low{0.0};
  double high{0.0};
  int64_t positive_seed_count{0};
};

struct PerSeedGeometry {
  double effective{0.0};
  double participation{0.0};
  double top{0.0};
  double active{0.0};
};

using GeometryBySeed = std::array<PerSeedGeometry, kSeedCount>;
using FamilyDeltas = std::array<double, kFamilyCount>;

struct VarianceNecessityGateInput {
  PairedContrast stratified_minus_jm{};
  PairedContrast variance_disabled_minus_stratified{};
  PairedContrast variance_disabled_minus_jm{};
  GeometryBySeed jm_geometry{};
  GeometryBySeed stratified_geometry{};
  GeometryBySeed variance_disabled_geometry{};
  FamilyDeltas variance_disabled_minus_jm_family{};
  bool mechanics{false};
};

enum class GateStatus { passed, failed, reference_not_reproduced };

struct GeometryGateResult {
  bool ratios_defined{false};
  double effective_ratio{std::numeric_limits<double>::quiet_NaN()};
  double participation_ratio{std::numeric_limits<double>::quiet_NaN()};
  double top_ratio{std::numeric_limits<double>::quiet_NaN()};
  bool effective_ratio_pass{false};
  bool participation_ratio_pass{false};
  bool top_ratio_pass{false};
  int64_t effective_direction_count{0};
  int64_t participation_direction_count{0};
  int64_t top_direction_count{0};
  bool effective_direction_pass{false};
  bool participation_direction_pass{false};
  bool top_direction_pass{false};
  std::array<bool, kSeedCount> candidate_active_seed_pass{};
  double candidate_min_active{std::numeric_limits<double>::quiet_NaN()};
  bool all_candidate_active_pass{false};
  bool pass{false};
};

struct VarianceNecessityGateResult {
  GateStatus status{GateStatus::failed};
  bool numeric_inputs_valid{false};
  bool mechanics_pass{false};
  bool harmful_aulc_reference_direction{false};
  bool geometry_reference_gaps_valid{false};
  bool reference_reproduced{false};
  bool reference_not_reproduced{false};
  bool rescue_point_pass{false};
  bool rescue_lower_bound_pass{false};
  bool rescue_positive_seed_count_pass{false};
  bool primary_rescue_pass{false};
  bool jm_noninferiority_pass{false};
  std::array<bool, kFamilyCount> family_delta_pass{};
  bool all_family_deltas_pass{false};
  GeometryGateResult geometry{};
  bool pass{false};
  // True only when the primary V0-S rescue passes but at least one other
  // scientific clause fails. Invalid or unreproduced-reference runs are never
  // classified as partial amelioration.
  bool partial_amelioration{false};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool unit_fraction(double value) {
  return finite(value) && value >= 0.0 && value <= 1.0;
}

[[nodiscard]] inline bool valid_contrast(const PairedContrast &contrast) {
  return finite(contrast.point) && finite(contrast.low) &&
         finite(contrast.high) && contrast.low <= contrast.high &&
         contrast.point >= contrast.low && contrast.point <= contrast.high &&
         contrast.positive_seed_count >= 0 &&
         contrast.positive_seed_count <= static_cast<int64_t>(kSeedCount);
}

[[nodiscard]] inline bool valid_geometry(const GeometryBySeed &geometry) {
  for (const auto &seed : geometry) {
    if (!unit_fraction(seed.effective) || !unit_fraction(seed.participation) ||
        !unit_fraction(seed.top) || !unit_fraction(seed.active)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool valid_families(const FamilyDeltas &families) {
  for (const auto delta : families) {
    if (!finite(delta)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline PerSeedGeometry
mean_geometry(const GeometryBySeed &geometry) {
  PerSeedGeometry mean{};
  for (const auto &seed : geometry) {
    mean.effective += seed.effective;
    mean.participation += seed.participation;
    mean.top += seed.top;
    mean.active += seed.active;
  }
  constexpr double denominator = static_cast<double>(kSeedCount);
  mean.effective /= denominator;
  mean.participation /= denominator;
  mean.top /= denominator;
  mean.active /= denominator;
  return mean;
}

[[nodiscard]] inline bool
valid_recovery_ratios(const GeometryBySeed &jm,
                      const GeometryBySeed &stratified,
                      const GeometryBySeed &candidate) {
  const auto jm_mean = mean_geometry(jm);
  const auto stratified_mean = mean_geometry(stratified);
  const auto candidate_mean = mean_geometry(candidate);
  const double effective_ratio =
      (candidate_mean.effective - stratified_mean.effective) /
      (jm_mean.effective - stratified_mean.effective);
  const double participation_ratio =
      (candidate_mean.participation - stratified_mean.participation) /
      (jm_mean.participation - stratified_mean.participation);
  const double top_ratio = (stratified_mean.top - candidate_mean.top) /
                           (stratified_mean.top - jm_mean.top);
  // Ratios may validly be negative or exceed one. Only their finiteness is a
  // numeric-domain contract; direction and recovery magnitude are gates.
  return finite(effective_ratio) && finite(participation_ratio) &&
         finite(top_ratio);
}

inline void evaluate_family_clauses(const FamilyDeltas &families,
                                    VarianceNecessityGateResult &result) {
  result.all_family_deltas_pass = true;
  for (std::size_t family = 0; family < kFamilyCount; ++family) {
    result.family_delta_pass[family] = families[family] >= -0.02;
    result.all_family_deltas_pass =
        result.all_family_deltas_pass && result.family_delta_pass[family];
  }
}

[[nodiscard]] inline GeometryGateResult
evaluate_geometry(const GeometryBySeed &jm, const GeometryBySeed &stratified,
                  const GeometryBySeed &candidate, bool numeric_inputs_valid,
                  bool reference_reproduced) {
  GeometryGateResult result{};
  if (!numeric_inputs_valid) {
    return result;
  }

  const auto jm_mean = mean_geometry(jm);
  const auto stratified_mean = mean_geometry(stratified);
  const auto candidate_mean = mean_geometry(candidate);
  result.ratios_defined = reference_reproduced;
  if (result.ratios_defined) {
    result.effective_ratio =
        (candidate_mean.effective - stratified_mean.effective) /
        (jm_mean.effective - stratified_mean.effective);
    result.participation_ratio =
        (candidate_mean.participation - stratified_mean.participation) /
        (jm_mean.participation - stratified_mean.participation);
    result.top_ratio = (stratified_mean.top - candidate_mean.top) /
                       (stratified_mean.top - jm_mean.top);
    result.effective_ratio_pass = result.effective_ratio >= 0.50;
    result.participation_ratio_pass = result.participation_ratio >= 0.50;
    result.top_ratio_pass = result.top_ratio >= 0.50;
  }

  result.all_candidate_active_pass = true;
  result.candidate_min_active = candidate.front().active;
  for (std::size_t seed = 0; seed < kSeedCount; ++seed) {
    result.effective_direction_count +=
        candidate[seed].effective > stratified[seed].effective ? 1 : 0;
    result.participation_direction_count +=
        candidate[seed].participation > stratified[seed].participation ? 1 : 0;
    result.top_direction_count +=
        candidate[seed].top < stratified[seed].top ? 1 : 0;
    result.candidate_active_seed_pass[seed] = candidate[seed].active >= 0.75;
    result.all_candidate_active_pass = result.all_candidate_active_pass &&
                                       result.candidate_active_seed_pass[seed];
    result.candidate_min_active =
        std::min(result.candidate_min_active, candidate[seed].active);
  }
  result.effective_direction_pass = result.effective_direction_count >= 2;
  result.participation_direction_pass =
      result.participation_direction_count >= 2;
  result.top_direction_pass = result.top_direction_count >= 2;
  result.pass = result.ratios_defined && result.effective_ratio_pass &&
                result.participation_ratio_pass && result.top_ratio_pass &&
                result.effective_direction_pass &&
                result.participation_direction_pass &&
                result.top_direction_pass && result.all_candidate_active_pass;
  return result;
}

} // namespace detail

[[nodiscard]] inline VarianceNecessityGateResult
evaluate_variance_necessity_gate(const VarianceNecessityGateInput &input) {
  VarianceNecessityGateResult result{};
  result.numeric_inputs_valid =
      detail::valid_contrast(input.stratified_minus_jm) &&
      detail::valid_contrast(input.variance_disabled_minus_stratified) &&
      detail::valid_contrast(input.variance_disabled_minus_jm) &&
      detail::valid_geometry(input.jm_geometry) &&
      detail::valid_geometry(input.stratified_geometry) &&
      detail::valid_geometry(input.variance_disabled_geometry) &&
      detail::valid_families(input.variance_disabled_minus_jm_family);
  result.mechanics_pass = input.mechanics;

  if (result.numeric_inputs_valid) {
    const auto jm_mean = detail::mean_geometry(input.jm_geometry);
    const auto stratified_mean =
        detail::mean_geometry(input.stratified_geometry);
    result.harmful_aulc_reference_direction =
        input.stratified_minus_jm.point < 0.0;
    result.geometry_reference_gaps_valid =
        jm_mean.effective > stratified_mean.effective &&
        jm_mean.participation > stratified_mean.participation &&
        stratified_mean.top > jm_mean.top;
    if (result.geometry_reference_gaps_valid &&
        !detail::valid_recovery_ratios(input.jm_geometry,
                                       input.stratified_geometry,
                                       input.variance_disabled_geometry)) {
      result.numeric_inputs_valid = false;
    }
    result.reference_reproduced = result.numeric_inputs_valid &&
                                  result.harmful_aulc_reference_direction &&
                                  result.geometry_reference_gaps_valid;
  }
  result.reference_not_reproduced = result.numeric_inputs_valid &&
                                    result.mechanics_pass &&
                                    !result.reference_reproduced;

  result.rescue_point_pass =
      input.variance_disabled_minus_stratified.point >= 0.0024;
  result.rescue_lower_bound_pass =
      input.variance_disabled_minus_stratified.low > 0.0;
  result.rescue_positive_seed_count_pass =
      input.variance_disabled_minus_stratified.positive_seed_count >= 2;
  result.primary_rescue_pass = result.rescue_point_pass &&
                               result.rescue_lower_bound_pass &&
                               result.rescue_positive_seed_count_pass;
  result.jm_noninferiority_pass = input.variance_disabled_minus_jm.low > -0.005;
  detail::evaluate_family_clauses(input.variance_disabled_minus_jm_family,
                                  result);
  result.geometry = detail::evaluate_geometry(
      input.jm_geometry, input.stratified_geometry,
      input.variance_disabled_geometry, result.numeric_inputs_valid,
      result.reference_reproduced);

  result.pass = result.numeric_inputs_valid && result.mechanics_pass &&
                result.reference_reproduced && result.primary_rescue_pass &&
                result.jm_noninferiority_pass &&
                result.all_family_deltas_pass && result.geometry.pass;
  result.partial_amelioration =
      result.numeric_inputs_valid && result.mechanics_pass &&
      result.reference_reproduced && result.primary_rescue_pass && !result.pass;

  if (!result.numeric_inputs_valid || !result.mechanics_pass) {
    result.status = GateStatus::failed;
  } else if (!result.reference_reproduced) {
    result.status = GateStatus::reference_not_reproduced;
  } else {
    result.status = result.pass ? GateStatus::passed : GateStatus::failed;
  }
  return result;
}

} // namespace cuwacunu::tests::mtf_vicreg_variance_necessity_gate
