#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace cuwacunu::tests::mtf_objective_repair_gate {

constexpr std::size_t kRepairSeedCount = 3;
constexpr std::size_t kRepairFamilyCount = 4;

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

using GeometryBySeed = std::array<PerSeedGeometry, kRepairSeedCount>;
using FamilyDeltas = std::array<double, kRepairFamilyCount>;
using TfRatios = std::array<double, kRepairSeedCount>;

struct TfRepairGateInput {
  PairedContrast fixed_tf_minus_jm{};
  PairedContrast candidate_minus_fixed_tf{};
  PairedContrast candidate_minus_jm{};
  GeometryBySeed jm_geometry{};
  GeometryBySeed fixed_tf_geometry{};
  GeometryBySeed candidate_geometry{};
  FamilyDeltas candidate_minus_jm_family{};
  TfRatios tf_weighted_norm_ratios{};
  bool mechanics{false};
};

struct VicregRepairGateInput {
  PairedContrast global_vicreg_minus_jm{};
  PairedContrast candidate_minus_global_vicreg{};
  PairedContrast candidate_minus_jm{};
  GeometryBySeed jm_geometry{};
  GeometryBySeed global_vicreg_geometry{};
  GeometryBySeed candidate_geometry{};
  FamilyDeltas candidate_minus_jm_family{};
  bool mechanics{false};
};

enum class RepairGateStatus { passed, failed, reference_not_reproduced };

struct GeometryGateResult {
  bool denominators_valid{false};
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
  double candidate_active_mean{std::numeric_limits<double>::quiet_NaN()};
  bool active_pass{false};
  bool pass{false};
};

struct CommonRepairGateResult {
  RepairGateStatus status{RepairGateStatus::failed};
  bool numeric_inputs_valid{false};
  bool harmful_reference_direction{false};
  bool reference_not_reproduced{false};
  bool repair_point_pass{false};
  bool repair_lower_bound_pass{false};
  bool repair_positive_seed_count_pass{false};
  bool jm_noninferiority_pass{false};
  std::array<bool, kRepairFamilyCount> family_delta_pass{};
  bool all_family_deltas_pass{false};
  bool mechanics_pass{false};
  GeometryGateResult geometry{};
};

struct TfRepairGateResult {
  CommonRepairGateResult common{};
  double tf_ratio_mean{std::numeric_limits<double>::quiet_NaN()};
  bool tf_ratio_mean_pass{false};
  std::array<bool, kRepairSeedCount> tf_ratio_seed_pass{};
  bool all_tf_ratio_seeds_pass{false};
  bool pass{false};
};

struct VicregRepairGateResult {
  CommonRepairGateResult common{};
  bool pass{false};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool valid_contrast(const PairedContrast &contrast) {
  return finite(contrast.point) && finite(contrast.low) &&
         finite(contrast.high) && contrast.low <= contrast.high &&
         contrast.positive_seed_count >= 0 &&
         contrast.positive_seed_count <= static_cast<int64_t>(kRepairSeedCount);
}

[[nodiscard]] inline bool valid_geometry(const GeometryBySeed &geometry) {
  for (const auto &seed : geometry) {
    if (!finite(seed.effective) || !finite(seed.participation) ||
        !finite(seed.top) || !finite(seed.active)) {
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
  constexpr double denominator = static_cast<double>(kRepairSeedCount);
  mean.effective /= denominator;
  mean.participation /= denominator;
  mean.top /= denominator;
  mean.active /= denominator;
  return mean;
}

[[nodiscard]] inline GeometryGateResult
evaluate_geometry(const GeometryBySeed &jm, const GeometryBySeed &harmful,
                  const GeometryBySeed &candidate, bool tf_mode,
                  bool harmful_reference_direction) {
  GeometryGateResult result{};
  const auto jm_mean = mean_geometry(jm);
  const auto harmful_mean = mean_geometry(harmful);
  const auto candidate_mean = mean_geometry(candidate);
  const double effective_denominator =
      tf_mode ? harmful_mean.effective - jm_mean.effective
              : jm_mean.effective - harmful_mean.effective;
  const double participation_denominator =
      tf_mode ? harmful_mean.participation - jm_mean.participation
              : jm_mean.participation - harmful_mean.participation;
  const double top_denominator =
      tf_mode ? jm_mean.top - harmful_mean.top : harmful_mean.top - jm_mean.top;
  result.denominators_valid = effective_denominator > 0.0 &&
                              participation_denominator > 0.0 &&
                              top_denominator > 0.0;
  result.ratios_defined =
      harmful_reference_direction && result.denominators_valid;
  if (result.ratios_defined) {
    result.effective_ratio =
        tf_mode ? (candidate_mean.effective - jm_mean.effective) /
                      effective_denominator
                : (candidate_mean.effective - harmful_mean.effective) /
                      effective_denominator;
    result.participation_ratio =
        tf_mode ? (candidate_mean.participation - jm_mean.participation) /
                      participation_denominator
                : (candidate_mean.participation - harmful_mean.participation) /
                      participation_denominator;
    result.top_ratio =
        tf_mode ? (jm_mean.top - candidate_mean.top) / top_denominator
                : (harmful_mean.top - candidate_mean.top) / top_denominator;
    result.effective_ratio_pass = result.effective_ratio >= 0.50;
    result.participation_ratio_pass = result.participation_ratio >= 0.50;
    result.top_ratio_pass = result.top_ratio >= 0.50;
  }

  for (std::size_t seed = 0; seed < kRepairSeedCount; ++seed) {
    const auto &direction_reference = tf_mode ? jm[seed] : harmful[seed];
    result.effective_direction_count +=
        candidate[seed].effective > direction_reference.effective ? 1 : 0;
    result.participation_direction_count +=
        candidate[seed].participation > direction_reference.participation ? 1
                                                                          : 0;
    result.top_direction_count +=
        candidate[seed].top < direction_reference.top ? 1 : 0;
  }
  result.effective_direction_pass = result.effective_direction_count >= 2;
  result.participation_direction_pass =
      result.participation_direction_count >= 2;
  result.top_direction_pass = result.top_direction_count >= 2;
  result.candidate_active_mean = candidate_mean.active;
  result.active_pass = result.candidate_active_mean >= 0.75;
  result.pass = result.ratios_defined && result.effective_ratio_pass &&
                result.participation_ratio_pass && result.top_ratio_pass &&
                result.effective_direction_pass &&
                result.participation_direction_pass &&
                result.top_direction_pass && result.active_pass;
  return result;
}

inline void evaluate_family_clauses(const FamilyDeltas &families,
                                    CommonRepairGateResult &result) {
  result.all_family_deltas_pass = true;
  for (std::size_t family = 0; family < kRepairFamilyCount; ++family) {
    result.family_delta_pass[family] = families[family] >= -0.02;
    result.all_family_deltas_pass =
        result.all_family_deltas_pass && result.family_delta_pass[family];
  }
}

[[nodiscard]] inline RepairGateStatus final_status(bool pass,
                                                   bool numeric_inputs_valid,
                                                   bool mechanics_pass,
                                                   bool reference_reversed) {
  if (!numeric_inputs_valid || !mechanics_pass) {
    return RepairGateStatus::failed;
  }
  if (reference_reversed) {
    return RepairGateStatus::reference_not_reproduced;
  }
  return pass ? RepairGateStatus::passed : RepairGateStatus::failed;
}

} // namespace detail

[[nodiscard]] inline TfRepairGateResult
evaluate_tf_repair_gate(const TfRepairGateInput &input) {
  TfRepairGateResult result{};
  auto &common = result.common;
  bool ratios_valid = true;
  double ratio_sum = 0.0;
  for (std::size_t seed = 0; seed < kRepairSeedCount; ++seed) {
    const double ratio = input.tf_weighted_norm_ratios[seed];
    ratios_valid = ratios_valid && detail::finite(ratio);
    ratio_sum += ratio;
    result.tf_ratio_seed_pass[seed] =
        detail::finite(ratio) && ratio >= 0.50 && ratio <= 2.00;
    result.all_tf_ratio_seeds_pass =
        seed == 0
            ? result.tf_ratio_seed_pass[seed]
            : result.all_tf_ratio_seeds_pass && result.tf_ratio_seed_pass[seed];
  }
  result.tf_ratio_mean = ratio_sum / static_cast<double>(kRepairSeedCount);
  result.tf_ratio_mean_pass = ratios_valid && result.tf_ratio_mean >= 0.80 &&
                              result.tf_ratio_mean <= 1.25;
  common.numeric_inputs_valid =
      detail::valid_contrast(input.fixed_tf_minus_jm) &&
      detail::valid_contrast(input.candidate_minus_fixed_tf) &&
      detail::valid_contrast(input.candidate_minus_jm) &&
      detail::valid_geometry(input.jm_geometry) &&
      detail::valid_geometry(input.fixed_tf_geometry) &&
      detail::valid_geometry(input.candidate_geometry) &&
      detail::valid_families(input.candidate_minus_jm_family) && ratios_valid;
  common.harmful_reference_direction =
      common.numeric_inputs_valid && input.fixed_tf_minus_jm.point < 0.0;
  common.mechanics_pass = input.mechanics;
  common.reference_not_reproduced = common.numeric_inputs_valid &&
                                    common.mechanics_pass &&
                                    !common.harmful_reference_direction;
  common.repair_point_pass = input.candidate_minus_fixed_tf.point >= 0.0044;
  common.repair_lower_bound_pass = input.candidate_minus_fixed_tf.low > 0.0;
  common.repair_positive_seed_count_pass =
      input.candidate_minus_fixed_tf.positive_seed_count >= 2;
  common.jm_noninferiority_pass = input.candidate_minus_jm.low > -0.005;
  detail::evaluate_family_clauses(input.candidate_minus_jm_family, common);
  common.geometry = detail::evaluate_geometry(
      input.jm_geometry, input.fixed_tf_geometry, input.candidate_geometry,
      /*tf_mode=*/true, common.harmful_reference_direction);
  result.pass =
      common.numeric_inputs_valid && common.harmful_reference_direction &&
      common.repair_point_pass && common.repair_lower_bound_pass &&
      common.repair_positive_seed_count_pass && common.jm_noninferiority_pass &&
      common.all_family_deltas_pass && common.geometry.pass &&
      result.tf_ratio_mean_pass && result.all_tf_ratio_seeds_pass &&
      common.mechanics_pass;
  common.status = detail::final_status(result.pass, common.numeric_inputs_valid,
                                       common.mechanics_pass,
                                       common.reference_not_reproduced);
  return result;
}

[[nodiscard]] inline VicregRepairGateResult
evaluate_vicreg_repair_gate(const VicregRepairGateInput &input) {
  VicregRepairGateResult result{};
  auto &common = result.common;
  common.numeric_inputs_valid =
      detail::valid_contrast(input.global_vicreg_minus_jm) &&
      detail::valid_contrast(input.candidate_minus_global_vicreg) &&
      detail::valid_contrast(input.candidate_minus_jm) &&
      detail::valid_geometry(input.jm_geometry) &&
      detail::valid_geometry(input.global_vicreg_geometry) &&
      detail::valid_geometry(input.candidate_geometry) &&
      detail::valid_families(input.candidate_minus_jm_family);
  common.harmful_reference_direction =
      common.numeric_inputs_valid && input.global_vicreg_minus_jm.point < 0.0;
  common.mechanics_pass = input.mechanics;
  common.reference_not_reproduced = common.numeric_inputs_valid &&
                                    common.mechanics_pass &&
                                    !common.harmful_reference_direction;
  common.repair_point_pass =
      input.candidate_minus_global_vicreg.point >= 0.0030;
  common.repair_lower_bound_pass =
      input.candidate_minus_global_vicreg.low > 0.0;
  common.repair_positive_seed_count_pass =
      input.candidate_minus_global_vicreg.positive_seed_count >= 2;
  common.jm_noninferiority_pass = input.candidate_minus_jm.low > -0.005;
  detail::evaluate_family_clauses(input.candidate_minus_jm_family, common);
  common.geometry = detail::evaluate_geometry(
      input.jm_geometry, input.global_vicreg_geometry, input.candidate_geometry,
      /*tf_mode=*/false, common.harmful_reference_direction);
  result.pass =
      common.numeric_inputs_valid && common.harmful_reference_direction &&
      common.repair_point_pass && common.repair_lower_bound_pass &&
      common.repair_positive_seed_count_pass && common.jm_noninferiority_pass &&
      common.all_family_deltas_pass && common.geometry.pass &&
      common.mechanics_pass;
  common.status = detail::final_status(result.pass, common.numeric_inputs_valid,
                                       common.mechanics_pass,
                                       common.reference_not_reproduced);
  return result;
}

} // namespace cuwacunu::tests::mtf_objective_repair_gate
