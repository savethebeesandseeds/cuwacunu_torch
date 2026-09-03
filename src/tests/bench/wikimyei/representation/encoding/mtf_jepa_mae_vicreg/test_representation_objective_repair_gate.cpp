#include "representation_objective_repair_gate.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace gate = cuwacunu::tests::mtf_objective_repair_gate;

namespace {

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] gate::GeometryBySeed repeated_geometry(double effective,
                                                     double participation,
                                                     double top,
                                                     double active = 0.75) {
  return {{{effective, participation, top, active},
           {effective, participation, top, active},
           {effective, participation, top, active}}};
}

[[nodiscard]] gate::TfRepairGateInput passing_tf_input() {
  return {
      .fixed_tf_minus_jm = {-0.010, -0.015, -0.005, 0},
      .candidate_minus_fixed_tf = {0.0044, 0.0001, 0.009, 2},
      .candidate_minus_jm = {-0.002, -0.0049, 0.002, 1},
      .jm_geometry = repeated_geometry(0.125, 0.125, 0.75),
      .fixed_tf_geometry = repeated_geometry(0.25, 0.25, 0.50),
      .candidate_geometry = repeated_geometry(0.1875, 0.1875, 0.625),
      .candidate_minus_jm_family = {-0.02, -0.01, 0.00, 0.01},
      .tf_weighted_norm_ratios = {0.80, 1.00, 1.25},
      .mechanics = true,
  };
}

[[nodiscard]] gate::VicregRepairGateInput passing_vicreg_input() {
  return {
      .global_vicreg_minus_jm = {-0.008, -0.012, -0.004, 0},
      .candidate_minus_global_vicreg = {0.0030, 0.0001, 0.007, 2},
      .candidate_minus_jm = {-0.002, -0.0049, 0.002, 1},
      .jm_geometry = repeated_geometry(0.375, 0.375, 0.625),
      .global_vicreg_geometry = repeated_geometry(0.125, 0.125, 0.875),
      .candidate_geometry = repeated_geometry(0.25, 0.25, 0.75),
      .candidate_minus_jm_family = {-0.02, -0.01, 0.00, 0.01},
      .mechanics = true,
  };
}

void test_exact_passing_fixtures() {
  const auto tf = gate::evaluate_tf_repair_gate(passing_tf_input());
  require(tf.pass, "exact TF boundary fixture must pass");
  require(tf.common.status == gate::RepairGateStatus::passed,
          "TF passing status mismatch");
  require(tf.common.geometry.effective_ratio_pass &&
              tf.common.geometry.participation_ratio_pass &&
              tf.common.geometry.top_ratio_pass,
          "TF geometry ratio boundary must pass");
  require(std::abs(tf.common.geometry.effective_ratio - 0.50) < 1.0e-12 &&
              std::abs(tf.common.geometry.participation_ratio - 0.50) <
                  1.0e-12 &&
              std::abs(tf.common.geometry.top_ratio - 0.50) < 1.0e-12,
          "TF exact half-retention ratios mismatch");
  require(tf.common.geometry.effective_direction_count == 3 &&
              tf.common.geometry.participation_direction_count == 3 &&
              tf.common.geometry.top_direction_count == 3,
          "TF direction counts mismatch");
  require(tf.common.geometry.candidate_active_mean == 0.75,
          "TF active boundary mismatch");

  const auto vicreg = gate::evaluate_vicreg_repair_gate(passing_vicreg_input());
  require(vicreg.pass, "exact VICReg boundary fixture must pass");
  require(vicreg.common.status == gate::RepairGateStatus::passed,
          "VICReg passing status mismatch");
  require(std::abs(vicreg.common.geometry.effective_ratio - 0.50) < 1.0e-12 &&
              std::abs(vicreg.common.geometry.participation_ratio - 0.50) <
                  1.0e-12 &&
              std::abs(vicreg.common.geometry.top_ratio - 0.50) < 1.0e-12,
          "VICReg exact half-recovery ratios mismatch");
}

void test_contrast_and_family_boundaries() {
  {
    auto input = passing_tf_input();
    input.candidate_minus_fixed_tf.point =
        std::nextafter(0.0044, -std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.repair_point_pass,
            "TF repair point below inclusive boundary must fail");
  }
  {
    auto input = passing_vicreg_input();
    input.candidate_minus_global_vicreg.point =
        std::nextafter(0.0030, -std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(!result.pass && !result.common.repair_point_pass,
            "VICReg repair point below inclusive boundary must fail");
  }
  for (const bool tf_mode : {true, false}) {
    if (tf_mode) {
      auto input = passing_tf_input();
      input.candidate_minus_fixed_tf.low = 0.0;
      auto result = gate::evaluate_tf_repair_gate(input);
      require(!result.pass && !result.common.repair_lower_bound_pass,
              "TF zero lower bound must fail strict positivity");
      input = passing_tf_input();
      input.candidate_minus_fixed_tf.positive_seed_count = 1;
      result = gate::evaluate_tf_repair_gate(input);
      require(!result.pass && !result.common.repair_positive_seed_count_pass,
              "TF one positive seed must fail");
    } else {
      auto input = passing_vicreg_input();
      input.candidate_minus_global_vicreg.low = 0.0;
      auto result = gate::evaluate_vicreg_repair_gate(input);
      require(!result.pass && !result.common.repair_lower_bound_pass,
              "VICReg zero lower bound must fail strict positivity");
      input = passing_vicreg_input();
      input.candidate_minus_global_vicreg.positive_seed_count = 1;
      result = gate::evaluate_vicreg_repair_gate(input);
      require(!result.pass && !result.common.repair_positive_seed_count_pass,
              "VICReg one positive seed must fail");
    }
  }
  {
    auto input = passing_tf_input();
    input.candidate_minus_jm.low = -0.005;
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.jm_noninferiority_pass,
            "TF exact negative noninferiority margin must fail");
  }
  {
    auto input = passing_vicreg_input();
    input.candidate_minus_jm.low = -0.005;
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(!result.pass && !result.common.jm_noninferiority_pass,
            "VICReg exact negative noninferiority margin must fail");
  }
  {
    auto input = passing_tf_input();
    input.candidate_minus_jm_family[2] =
        std::nextafter(-0.02, -std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.family_delta_pass[2] &&
                !result.common.all_family_deltas_pass,
            "family delta below inclusive floor must fail only that family");
  }
}

void test_reference_and_denominator_failures() {
  {
    auto input = passing_tf_input();
    input.fixed_tf_minus_jm.point = 0.0;
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && result.common.reference_not_reproduced &&
                result.common.status ==
                    gate::RepairGateStatus::reference_not_reproduced &&
                !result.common.geometry.ratios_defined,
            "TF reversed AULC reference must be classified and undefined");
  }
  {
    auto input = passing_vicreg_input();
    input.global_vicreg_minus_jm.point = 0.001;
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(!result.pass && result.common.reference_not_reproduced &&
                result.common.status ==
                    gate::RepairGateStatus::reference_not_reproduced &&
                !result.common.geometry.ratios_defined,
            "VICReg reversed AULC reference must be classified and undefined");
  }
  {
    auto input = passing_tf_input();
    input.fixed_tf_geometry = input.jm_geometry;
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.geometry.denominators_valid &&
                !result.common.geometry.ratios_defined,
            "zero TF geometry denominators must fail without a ratio");
  }
  {
    auto input = passing_vicreg_input();
    for (auto &seed : input.global_vicreg_geometry) {
      seed.effective = 0.50;
    }
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(!result.pass && !result.common.geometry.denominators_valid &&
                !result.common.geometry.ratios_defined,
            "reversed VICReg geometry denominator must fail without a ratio");
  }
}

void test_geometry_direction_and_active_clauses() {
  {
    auto input = passing_tf_input();
    input.candidate_geometry = {{{0.21875, 0.21875, 0.5625, 0.75},
                                 {0.21875, 0.21875, 0.5625, 0.75},
                                 {0.125, 0.125, 0.75, 0.75}}};
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(result.pass &&
                result.common.geometry.effective_direction_count == 2 &&
                result.common.geometry.participation_direction_count == 2 &&
                result.common.geometry.top_direction_count == 2,
            "TF exact two-of-three direction boundary must pass");
  }
  {
    auto input = passing_vicreg_input();
    input.candidate_geometry = {{{0.3125, 0.3125, 0.6875, 0.75},
                                 {0.3125, 0.3125, 0.6875, 0.75},
                                 {0.125, 0.125, 0.875, 0.75}}};
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(result.pass &&
                result.common.geometry.effective_direction_count == 2 &&
                result.common.geometry.participation_direction_count == 2 &&
                result.common.geometry.top_direction_count == 2,
            "VICReg exact two-of-three direction boundary must pass");
  }
  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_tf_input();
    if (metric == 0) {
      for (auto &seed : input.candidate_geometry) {
        seed.effective = 0.1874;
      }
    } else if (metric == 1) {
      for (auto &seed : input.candidate_geometry) {
        seed.participation = 0.1874;
      }
    } else {
      for (auto &seed : input.candidate_geometry) {
        seed.top = 0.6251;
      }
    }
    const auto result = gate::evaluate_tf_repair_gate(input);
    const bool failed_clause =
        metric == 0   ? !result.common.geometry.effective_ratio_pass
        : metric == 1 ? !result.common.geometry.participation_ratio_pass
                      : !result.common.geometry.top_ratio_pass;
    require(!result.pass && failed_clause,
            "TF geometry ratio below one-half must fail its metric");
  }
  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_vicreg_input();
    if (metric == 0) {
      for (auto &seed : input.candidate_geometry) {
        seed.effective = 0.2499;
      }
    } else if (metric == 1) {
      for (auto &seed : input.candidate_geometry) {
        seed.participation = 0.2499;
      }
    } else {
      for (auto &seed : input.candidate_geometry) {
        seed.top = 0.7501;
      }
    }
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    const bool failed_clause =
        metric == 0   ? !result.common.geometry.effective_ratio_pass
        : metric == 1 ? !result.common.geometry.participation_ratio_pass
                      : !result.common.geometry.top_ratio_pass;
    require(!result.pass && failed_clause,
            "VICReg geometry ratio below one-half must fail its metric");
  }
  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_tf_input();
    if (metric == 0) {
      input.candidate_geometry[0].effective = 0.3125;
      input.candidate_geometry[1].effective = 0.125;
      input.candidate_geometry[2].effective = 0.125;
    } else if (metric == 1) {
      input.candidate_geometry[0].participation = 0.3125;
      input.candidate_geometry[1].participation = 0.125;
      input.candidate_geometry[2].participation = 0.125;
    } else {
      input.candidate_geometry[0].top = 0.375;
      input.candidate_geometry[1].top = 0.75;
      input.candidate_geometry[2].top = 0.75;
    }
    const auto result = gate::evaluate_tf_repair_gate(input);
    const bool failed_clause =
        metric == 0   ? !result.common.geometry.effective_direction_pass
        : metric == 1 ? !result.common.geometry.participation_direction_pass
                      : !result.common.geometry.top_direction_pass;
    require(!result.pass && failed_clause,
            "TF one-of-three directional movement must fail its metric");
  }
  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_vicreg_input();
    if (metric == 0) {
      input.candidate_geometry[0].effective = 0.50;
      input.candidate_geometry[1].effective = 0.125;
      input.candidate_geometry[2].effective = 0.125;
    } else if (metric == 1) {
      input.candidate_geometry[0].participation = 0.50;
      input.candidate_geometry[1].participation = 0.125;
      input.candidate_geometry[2].participation = 0.125;
    } else {
      input.candidate_geometry[0].top = 0.50;
      input.candidate_geometry[1].top = 0.875;
      input.candidate_geometry[2].top = 0.875;
    }
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    const bool failed_clause =
        metric == 0   ? !result.common.geometry.effective_direction_pass
        : metric == 1 ? !result.common.geometry.participation_direction_pass
                      : !result.common.geometry.top_direction_pass;
    require(!result.pass && failed_clause,
            "VICReg one-of-three directional movement must fail its metric");
  }
  {
    auto input = passing_tf_input();
    for (auto &seed : input.candidate_geometry) {
      seed.active = 0.749;
    }
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.geometry.active_pass,
            "fixed-seed active mean below 0.75 must fail");
  }
  {
    auto input = passing_vicreg_input();
    for (auto &seed : input.candidate_geometry) {
      seed.active = 0.749;
    }
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(!result.pass && !result.common.geometry.active_pass,
            "VICReg fixed-seed active mean below 0.75 must fail");
  }
}

void test_tf_ratio_boundaries() {
  {
    auto input = passing_tf_input();
    input.tf_weighted_norm_ratios = {0.50, 0.80, 2.00};
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(result.pass && result.all_tf_ratio_seeds_pass,
            "inclusive TF per-seed ratio boundaries must pass");
  }
  for (const double mean_boundary : {0.80, 1.25}) {
    auto input = passing_tf_input();
    input.tf_weighted_norm_ratios = {mean_boundary, mean_boundary,
                                     mean_boundary};
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(result.pass && result.tf_ratio_mean_pass,
            "inclusive TF ratio-mean boundary must pass");
  }
  {
    auto input = passing_tf_input();
    input.tf_weighted_norm_ratios = {0.49, 1.00, 1.00};
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.all_tf_ratio_seeds_pass &&
                result.tf_ratio_mean_pass,
            "TF per-seed lower violation must fail independently");
  }
  {
    auto input = passing_tf_input();
    input.tf_weighted_norm_ratios = {2.01, 0.80, 0.80};
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.all_tf_ratio_seeds_pass &&
                result.tf_ratio_mean_pass,
            "TF per-seed upper violation must fail independently");
  }
  for (const double mean_violation : {0.79, 1.26}) {
    auto input = passing_tf_input();
    input.tf_weighted_norm_ratios = {mean_violation, mean_violation,
                                     mean_violation};
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.tf_ratio_mean_pass,
            "TF ratio mean outside frozen range must fail");
  }
}

void test_mechanics_and_invalid_numeric_input() {
  {
    auto input = passing_tf_input();
    input.mechanics = false;
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.mechanics_pass,
            "TF mechanics failure must fail the gate");
  }
  {
    auto input = passing_vicreg_input();
    input.mechanics = false;
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(!result.pass && !result.common.mechanics_pass,
            "VICReg mechanics failure must fail the gate");
  }
  {
    auto input = passing_tf_input();
    input.fixed_tf_minus_jm.point = 0.001;
    input.mechanics = false;
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.reference_not_reproduced &&
                result.common.status == gate::RepairGateStatus::failed,
            "invalid TF mechanics must take precedence over reference "
            "classification");
  }
  {
    auto input = passing_vicreg_input();
    input.global_vicreg_minus_jm.point = 0.001;
    input.mechanics = false;
    const auto result = gate::evaluate_vicreg_repair_gate(input);
    require(!result.pass && !result.common.reference_not_reproduced &&
                result.common.status == gate::RepairGateStatus::failed,
            "invalid VICReg mechanics must take precedence over reference "
            "classification");
  }
  {
    auto input = passing_tf_input();
    input.candidate_geometry[1].top = std::numeric_limits<double>::quiet_NaN();
    const auto result = gate::evaluate_tf_repair_gate(input);
    require(!result.pass && !result.common.numeric_inputs_valid &&
                result.common.status == gate::RepairGateStatus::failed,
            "non-finite input must fail without reference reversal label");
  }
}

} // namespace

int main() {
  try {
    test_exact_passing_fixtures();
    test_contrast_and_family_boundaries();
    test_reference_and_denominator_failures();
    test_geometry_direction_and_active_clauses();
    test_tf_ratio_boundaries();
    test_mechanics_and_invalid_numeric_input();
    std::cout << "representation objective repair gate tests passed\n";
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << "representation objective repair gate tests failed: "
              << exception.what() << '\n';
    return 1;
  }
}
