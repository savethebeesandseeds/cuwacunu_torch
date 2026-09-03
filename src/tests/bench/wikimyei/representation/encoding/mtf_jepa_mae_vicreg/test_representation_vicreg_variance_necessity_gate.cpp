#include "representation_vicreg_variance_necessity_gate.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace gate = cuwacunu::tests::mtf_vicreg_variance_necessity_gate;

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

[[nodiscard]] gate::VarianceNecessityGateInput passing_input() {
  return {
      .stratified_minus_jm = {-0.0048, -0.010, 0.0005, 0},
      .variance_disabled_minus_stratified = {0.0024, 0.0001, 0.006, 2},
      .variance_disabled_minus_jm = {-0.0024, -0.0049, 0.001, 1},
      .jm_geometry = repeated_geometry(0.375, 0.375, 0.625),
      .stratified_geometry = repeated_geometry(0.125, 0.125, 0.875),
      .variance_disabled_geometry = repeated_geometry(0.25, 0.25, 0.75),
      .variance_disabled_minus_jm_family = {-0.02, -0.01, 0.00, 0.01},
      .mechanics = true,
  };
}

void test_exact_passing_fixture() {
  const auto result = gate::evaluate_variance_necessity_gate(passing_input());
  require(result.pass && result.status == gate::GateStatus::passed,
          "exact boundary fixture must pass");
  require(result.numeric_inputs_valid && result.mechanics_pass &&
              result.harmful_aulc_reference_direction &&
              result.geometry_reference_gaps_valid &&
              result.reference_reproduced && !result.reference_not_reproduced,
          "passing reference classification mismatch");
  require(result.primary_rescue_pass && result.jm_noninferiority_pass &&
              result.all_family_deltas_pass && result.geometry.pass,
          "passing conjunctive clauses mismatch");
  require(std::abs(result.geometry.effective_ratio - 0.50) < 1.0e-12 &&
              std::abs(result.geometry.participation_ratio - 0.50) < 1.0e-12 &&
              std::abs(result.geometry.top_ratio - 0.50) < 1.0e-12,
          "exact half-recovery ratios mismatch");
  require(result.geometry.effective_direction_count == 3 &&
              result.geometry.participation_direction_count == 3 &&
              result.geometry.top_direction_count == 3,
          "passing direction counts mismatch");
  require(result.geometry.candidate_min_active == 0.75 &&
              result.geometry.all_candidate_active_pass,
          "inclusive per-seed active boundary must pass");
  require(!result.partial_amelioration,
          "a full pass must not be partial amelioration");
}

void test_authoritative_result_unchanged() {
  auto input = gate::VarianceNecessityGateInput{
      .stratified_minus_jm = {-0.0048333277913491512, -0.010301600955106703,
                              0.0005726221595940853, 0},
      .variance_disabled_minus_stratified = {0.0048333651083776923,
                                             -0.00057273165545246166,
                                             0.010302027186482236, 3},
      .variance_disabled_minus_jm = {3.7317028541335638e-08,
                                     -6.1710127661947687e-07,
                                     6.1376762998064747e-07, 2},
      .jm_geometry = repeated_geometry(
          0.098293117829745383, 0.067797022134521115, 0.75127318743580318, 1.0),
      .stratified_geometry = repeated_geometry(
          0.057860468444975714, 0.04227565970812236, 0.90750901060780509, 1.0),
      .variance_disabled_geometry = repeated_geometry(
          0.098293188925567113, 0.06779706429665254, 0.75127310706881112, 1.0),
      .variance_disabled_minus_jm_family = {2.5662905619080095e-07,
                                            -4.80587225456149e-06,
                                            3.7986821380675551e-06,
                                            -5.7165764836645627e-07},
      .mechanics = true,
  };
  const auto result = gate::evaluate_variance_necessity_gate(input);
  require(result.numeric_inputs_valid && result.reference_reproduced &&
              result.rescue_point_pass && !result.rescue_lower_bound_pass &&
              result.rescue_positive_seed_count_pass &&
              !result.primary_rescue_pass && result.jm_noninferiority_pass &&
              result.all_family_deltas_pass && result.geometry.pass &&
              !result.pass && !result.partial_amelioration &&
              result.status == gate::GateStatus::failed,
          "authoritative gate result must remain unchanged");
}

void test_primary_and_noninferiority_boundaries() {
  {
    auto input = passing_input();
    input.variance_disabled_minus_stratified.point =
        std::nextafter(0.0024, -std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.rescue_point_pass &&
                !result.primary_rescue_pass,
            "rescue point below inclusive boundary must fail");
  }
  {
    auto input = passing_input();
    input.variance_disabled_minus_stratified.low = 0.0;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.rescue_lower_bound_pass &&
                !result.primary_rescue_pass,
            "zero rescue lower bound must fail strict positivity");
  }
  {
    auto input = passing_input();
    input.variance_disabled_minus_stratified.positive_seed_count = 1;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.rescue_positive_seed_count_pass &&
                !result.primary_rescue_pass,
            "one positive rescue seed must fail");
  }
  {
    auto input = passing_input();
    input.variance_disabled_minus_jm.low = -0.005;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.jm_noninferiority_pass &&
                result.primary_rescue_pass && result.partial_amelioration,
            "exact noninferiority margin must fail as partial amelioration");
  }
  {
    auto input = passing_input();
    input.variance_disabled_minus_jm.low =
        std::nextafter(-0.005, std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(result.pass && result.jm_noninferiority_pass,
            "value above strict noninferiority margin must pass");
  }
}

void test_family_boundaries() {
  for (std::size_t family = 0; family < gate::kFamilyCount; ++family) {
    auto input = passing_input();
    input.variance_disabled_minus_jm_family[family] = -0.02;
    auto result = gate::evaluate_variance_necessity_gate(input);
    require(result.pass && result.family_delta_pass[family],
            "inclusive family floor must pass");

    input.variance_disabled_minus_jm_family[family] =
        std::nextafter(-0.02, -std::numeric_limits<double>::infinity());
    result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.family_delta_pass[family] &&
                !result.all_family_deltas_pass && result.partial_amelioration,
            "family value below floor must fail as partial amelioration");
  }
}

void test_reference_reproduction() {
  for (const double reversed_point : {0.0, 0.001}) {
    auto input = passing_input();
    input.stratified_minus_jm.point = reversed_point;
    input.stratified_minus_jm.high = 0.002;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.reference_reproduced &&
                result.reference_not_reproduced &&
                result.status == gate::GateStatus::reference_not_reproduced &&
                !result.geometry.ratios_defined && !result.partial_amelioration,
            "non-harmful AULC reference must be classified as unreproduced");
  }

  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_input();
    if (metric == 0) {
      for (auto &seed : input.stratified_geometry) {
        seed.effective = 0.375;
      }
    } else if (metric == 1) {
      for (auto &seed : input.stratified_geometry) {
        seed.participation = 0.40;
      }
    } else {
      for (auto &seed : input.stratified_geometry) {
        seed.top = 0.625;
      }
    }
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.geometry_reference_gaps_valid &&
                result.reference_not_reproduced &&
                result.status == gate::GateStatus::reference_not_reproduced &&
                !result.geometry.ratios_defined,
            "zero or reversed geometry gap must make reference unreproduced");
  }
}

void test_contrast_numeric_domains() {
  using Input = gate::VarianceNecessityGateInput;
  using ContrastMember = gate::PairedContrast Input::*;
  constexpr std::array<ContrastMember, 3> contrasts{
      &Input::stratified_minus_jm,
      &Input::variance_disabled_minus_stratified,
      &Input::variance_disabled_minus_jm,
  };
  for (const auto member : contrasts) {
    {
      auto input = passing_input();
      auto &contrast = input.*member;
      contrast.point = std::nextafter(contrast.low,
                                      -std::numeric_limits<double>::infinity());
      const auto result = gate::evaluate_variance_necessity_gate(input);
      require(!result.numeric_inputs_valid &&
                  result.status == gate::GateStatus::failed &&
                  !result.reference_not_reproduced,
              "contrast point below its interval must be invalid");
    }
    {
      auto input = passing_input();
      auto &contrast = input.*member;
      contrast.point = std::nextafter(contrast.high,
                                      std::numeric_limits<double>::infinity());
      const auto result = gate::evaluate_variance_necessity_gate(input);
      require(!result.numeric_inputs_valid &&
                  result.status == gate::GateStatus::failed &&
                  !result.reference_not_reproduced,
              "contrast point above its interval must be invalid");
    }
    for (const bool use_low : {true, false}) {
      auto input = passing_input();
      auto &contrast = input.*member;
      contrast.point = use_low ? contrast.low : contrast.high;
      const auto result = gate::evaluate_variance_necessity_gate(input);
      require(result.numeric_inputs_valid,
              "contrast point on an interval boundary must be valid");
    }
  }
}

void test_geometry_numeric_domains() {
  using Input = gate::VarianceNecessityGateInput;
  using GeometryMember = gate::GeometryBySeed Input::*;
  using FractionMember = double gate::PerSeedGeometry::*;
  constexpr std::array<GeometryMember, 3> geometries{
      &Input::jm_geometry,
      &Input::stratified_geometry,
      &Input::variance_disabled_geometry,
  };
  constexpr std::array<FractionMember, 4> fractions{
      &gate::PerSeedGeometry::effective,
      &gate::PerSeedGeometry::participation,
      &gate::PerSeedGeometry::top,
      &gate::PerSeedGeometry::active,
  };
  for (const auto geometry_member : geometries) {
    for (const auto fraction_member : fractions) {
      for (const bool below : {true, false}) {
        auto input = passing_input();
        auto &value = (input.*geometry_member)[1].*fraction_member;
        value =
            below
                ? std::nextafter(0.0, -std::numeric_limits<double>::infinity())
                : std::nextafter(1.0, std::numeric_limits<double>::infinity());
        const auto result = gate::evaluate_variance_necessity_gate(input);
        require(!result.numeric_inputs_valid &&
                    result.status == gate::GateStatus::failed &&
                    !result.reference_not_reproduced,
                "geometry fraction outside [0,1] must be invalid");
      }
    }
  }

  for (const auto fraction_member : fractions) {
    for (const double boundary : {0.0, 1.0}) {
      auto input = passing_input();
      input.variance_disabled_geometry[1].*fraction_member = boundary;
      const auto result = gate::evaluate_variance_necessity_gate(input);
      require(result.numeric_inputs_valid,
              "geometry fraction on [0,1] boundary must be valid");
    }
  }
}

void test_nonfinite_recovery_ratios() {
  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_input();
    const double tiny_gap = std::numeric_limits<double>::denorm_min();
    if (metric == 0) {
      input.stratified_geometry = repeated_geometry(0.0, 0.125, 0.875);
      input.jm_geometry = repeated_geometry(tiny_gap, 0.375, 0.625);
      for (auto &seed : input.variance_disabled_geometry) {
        seed.effective = 1.0;
      }
    } else if (metric == 1) {
      input.stratified_geometry = repeated_geometry(0.125, 0.0, 0.875);
      input.jm_geometry = repeated_geometry(0.375, tiny_gap, 0.625);
      for (auto &seed : input.variance_disabled_geometry) {
        seed.participation = 1.0;
      }
    } else {
      input.stratified_geometry = repeated_geometry(0.125, 0.125, tiny_gap);
      input.jm_geometry = repeated_geometry(0.375, 0.375, 0.0);
      for (auto &seed : input.variance_disabled_geometry) {
        seed.top = 1.0;
      }
    }
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.numeric_inputs_valid &&
                result.status == gate::GateStatus::failed &&
                !result.reference_not_reproduced &&
                !result.geometry.ratios_defined,
            "non-finite recovery ratio must invalidate numeric inputs");
  }
}

void test_geometry_ratio_boundaries() {
  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_input();
    const double below_half =
        std::nextafter(0.25, -std::numeric_limits<double>::infinity());
    if (metric == 0) {
      for (auto &seed : input.variance_disabled_geometry) {
        seed.effective = below_half;
      }
    } else if (metric == 1) {
      for (auto &seed : input.variance_disabled_geometry) {
        seed.participation = below_half;
      }
    } else {
      const double above_half =
          std::nextafter(0.75, std::numeric_limits<double>::infinity());
      for (auto &seed : input.variance_disabled_geometry) {
        seed.top = above_half;
      }
    }
    const auto result = gate::evaluate_variance_necessity_gate(input);
    const bool ratio_failed =
        metric == 0   ? !result.geometry.effective_ratio_pass
        : metric == 1 ? !result.geometry.participation_ratio_pass
                      : !result.geometry.top_ratio_pass;
    require(!result.pass && ratio_failed && result.partial_amelioration,
            "geometry ratio below one-half must fail its clause");
  }
}

void test_geometry_directions() {
  {
    auto input = passing_input();
    input.variance_disabled_geometry = {{{0.3125, 0.3125, 0.6875, 0.75},
                                         {0.3125, 0.3125, 0.6875, 0.75},
                                         {0.125, 0.125, 0.875, 0.75}}};
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(result.pass && result.geometry.effective_direction_count == 2 &&
                result.geometry.participation_direction_count == 2 &&
                result.geometry.top_direction_count == 2,
            "exact two-of-three direction boundary must pass");
  }

  for (int metric = 0; metric < 3; ++metric) {
    auto input = passing_input();
    if (metric == 0) {
      input.variance_disabled_geometry[0].effective = 0.50;
      input.variance_disabled_geometry[1].effective = 0.125;
      input.variance_disabled_geometry[2].effective = 0.125;
    } else if (metric == 1) {
      input.variance_disabled_geometry[0].participation = 0.50;
      input.variance_disabled_geometry[1].participation = 0.125;
      input.variance_disabled_geometry[2].participation = 0.125;
    } else {
      input.variance_disabled_geometry[0].top = 0.50;
      input.variance_disabled_geometry[1].top = 0.875;
      input.variance_disabled_geometry[2].top = 0.875;
    }
    const auto result = gate::evaluate_variance_necessity_gate(input);
    const bool direction_failed =
        metric == 0   ? !result.geometry.effective_direction_pass
        : metric == 1 ? !result.geometry.participation_direction_pass
                      : !result.geometry.top_direction_pass;
    require(!result.pass && direction_failed && result.partial_amelioration,
            "one-of-three directional movement must fail its clause");
  }
}

void test_per_seed_active_clause() {
  {
    auto input = passing_input();
    input.variance_disabled_geometry[1].active =
        std::nextafter(0.75, -std::numeric_limits<double>::infinity());
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.geometry.candidate_active_seed_pass[1] &&
                !result.geometry.all_candidate_active_pass &&
                result.partial_amelioration,
            "one seed below active floor must fail despite passing mean");
  }
  {
    auto input = passing_input();
    input.variance_disabled_geometry[0].active = 0.75;
    input.variance_disabled_geometry[1].active = 0.75;
    input.variance_disabled_geometry[2].active = 0.75;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(result.pass && result.geometry.all_candidate_active_pass,
            "all seeds at inclusive active floor must pass");
  }
}

void test_invalid_numeric_inputs() {
  {
    auto input = passing_input();
    input.stratified_minus_jm.point = std::numeric_limits<double>::quiet_NaN();
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.numeric_inputs_valid &&
                result.status == gate::GateStatus::failed &&
                !result.reference_not_reproduced &&
                !result.partial_amelioration,
            "non-finite contrast must be invalid, not unreproduced");
  }
  {
    auto input = passing_input();
    input.variance_disabled_minus_stratified.low = 0.01;
    input.variance_disabled_minus_stratified.high = 0.001;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.numeric_inputs_valid &&
                result.status == gate::GateStatus::failed,
            "reversed contrast interval must be invalid");
  }
  for (const int64_t invalid_count : {-1, 4}) {
    auto input = passing_input();
    input.variance_disabled_minus_jm.positive_seed_count = invalid_count;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.numeric_inputs_valid &&
                result.status == gate::GateStatus::failed,
            "out-of-range positive-seed count must be invalid");
  }
  {
    auto input = passing_input();
    input.variance_disabled_geometry[2].top =
        std::numeric_limits<double>::infinity();
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.numeric_inputs_valid && !result.geometry.ratios_defined &&
                result.status == gate::GateStatus::failed,
            "non-finite geometry must invalidate the gate");
  }
  {
    auto input = passing_input();
    input.variance_disabled_minus_jm_family[3] =
        -std::numeric_limits<double>::infinity();
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.numeric_inputs_valid &&
                result.status == gate::GateStatus::failed,
            "non-finite family delta must invalidate the gate");
  }
}

void test_invalid_mechanics_precedence() {
  {
    auto input = passing_input();
    input.mechanics = false;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.mechanics_pass &&
                result.status == gate::GateStatus::failed &&
                !result.partial_amelioration,
            "mechanics failure must fail the gate");
  }
  {
    auto input = passing_input();
    input.mechanics = false;
    input.stratified_minus_jm.point = 0.001;
    input.stratified_minus_jm.high = 0.002;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.pass && !result.reference_not_reproduced &&
                result.status == gate::GateStatus::failed,
            "invalid mechanics must precede reference reversal");
  }
  {
    auto input = passing_input();
    input.mechanics = false;
    for (auto &seed : input.stratified_geometry) {
      seed.effective = 0.50;
    }
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.reference_not_reproduced &&
                result.status == gate::GateStatus::failed,
            "invalid mechanics must precede geometry-reference failure");
  }
}

void test_partial_amelioration_helper() {
  {
    auto input = passing_input();
    input.variance_disabled_minus_jm.low = -0.006;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(result.primary_rescue_pass && !result.pass &&
                result.partial_amelioration &&
                result.status == gate::GateStatus::failed,
            "primary-only rescue must be identifiable as partial");
  }
  {
    auto input = passing_input();
    input.variance_disabled_minus_stratified.low = 0.0;
    const auto result = gate::evaluate_variance_necessity_gate(input);
    require(!result.primary_rescue_pass && !result.partial_amelioration,
            "failed primary rescue must not be partial amelioration");
  }
}

} // namespace

int main() {
  try {
    test_exact_passing_fixture();
    test_authoritative_result_unchanged();
    test_primary_and_noninferiority_boundaries();
    test_family_boundaries();
    test_reference_reproduction();
    test_contrast_numeric_domains();
    test_geometry_numeric_domains();
    test_nonfinite_recovery_ratios();
    test_geometry_ratio_boundaries();
    test_geometry_directions();
    test_per_seed_active_clause();
    test_invalid_numeric_inputs();
    test_invalid_mechanics_precedence();
    test_partial_amelioration_helper();
    std::cout << "VICReg variance necessity gate tests passed\n";
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << "VICReg variance necessity gate tests failed: "
              << exception.what() << '\n';
    return 1;
  }
}
