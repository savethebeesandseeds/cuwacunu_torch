#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace cuwacunu::tests::pooling_structure_mechanism_map_gate {

constexpr std::size_t kSeedCount = 3;
constexpr std::size_t kFamilyCount = 4;
constexpr std::size_t kCandidateCount = 3;
constexpr double kMaterialityThreshold = 0.02;
constexpr double kNoninferiorityMargin = -0.02;
constexpr double kFamilyNoninferiorityMargin = -0.05;
constexpr double kOrderDecodablePoint = 0.60;
constexpr double kOrderChance = 0.50;

using SeedValues = std::array<double, kSeedCount>;
using FamilyValues = std::array<double, kFamilyCount>;

struct ContinuousInput {
  double point{0.0};
  double low{0.0};
  double high{0.0};
  SeedValues seed_deltas{};
  FamilyValues family_deltas{};
};

enum class ContinuousClassification {
  invalid_numeric,
  material_gain,
  noninferior,
  unresolved,
};

[[nodiscard]] inline const char *continuous_classification_name(
    ContinuousClassification classification) {
  switch (classification) {
  case ContinuousClassification::invalid_numeric:
    return "invalid_numeric";
  case ContinuousClassification::material_gain:
    return "material_gain";
  case ContinuousClassification::noninferior:
    return "noninferior";
  case ContinuousClassification::unresolved:
    return "unresolved";
  }
  return "invalid_numeric";
}

struct ContinuousResult {
  bool numeric_valid{false};
  int64_t positive_seed_count{0};
  int64_t noninferior_seed_count{0};
  bool material_gain{false};
  bool noninferior{false};
  ContinuousClassification classification{
      ContinuousClassification::invalid_numeric};
};

struct OrderInput {
  double point{0.0};
  double low{0.0};
  double high{0.0};
  SeedValues seed_points{};
};

enum class OrderClassification {
  invalid_numeric,
  order_decodable,
  chance_consistent,
  order_unresolved,
};

[[nodiscard]] inline const char *
order_classification_name(OrderClassification classification) {
  switch (classification) {
  case OrderClassification::invalid_numeric:
    return "invalid_numeric";
  case OrderClassification::order_decodable:
    return "order_decodable";
  case OrderClassification::chance_consistent:
    return "chance_consistent";
  case OrderClassification::order_unresolved:
    return "order_unresolved";
  }
  return "invalid_numeric";
}

struct OrderResult {
  bool numeric_valid{false};
  int64_t above_chance_seed_count{0};
  OrderClassification classification{OrderClassification::invalid_numeric};
};

enum class Candidate : std::size_t {
  channel_domain = 0,
  channel_domain_scale = 1,
  channel_domain_scale_bin = 2,
};

[[nodiscard]] constexpr std::size_t candidate_index(Candidate candidate) {
  return static_cast<std::size_t>(candidate);
}

struct CandidateInput {
  ContinuousInput minus_encoder{};
  ContinuousInput minus_channel{};
  OrderInput order{};
};

struct CandidateResult {
  ContinuousResult minus_encoder{};
  ContinuousResult minus_channel{};
  OrderResult order{};
  bool continuous_restored{false};
  bool restored{false};
};

struct ValidityInput {
  bool no_training_or_end_to_end{false};
  bool capture_and_identity_exact{false};
  bool parameters_and_rng_unchanged{false};
  bool partitions_valid{false};
  bool projection_valid{false};
  bool deterministic_tables_valid{false};
  bool references_reproduced{false};
  bool shuffled_controls_pass{false};
};

struct GateInput {
  ContinuousInput encoder_minus_channel{};
  OrderInput encoder_order{};
  OrderInput channel_order{};
  std::array<CandidateInput, kCandidateCount> candidates{};
  ValidityInput validity{};
};

enum class TerminalClassification {
  invalid_mechanics,
  reference_reproduction_failure,
  encoder_boundary_not_reproduced,
  domain_separation_sufficient,
  domain_scale_separation_sufficient,
  coarse_position_separation_sufficient,
  factors_restored_order_not_restored,
  order_restored_factors_not_restored,
  fixed_summaries_not_sufficient,
};

[[nodiscard]] inline const char *
terminal_classification_name(TerminalClassification classification) {
  switch (classification) {
  case TerminalClassification::invalid_mechanics:
    return "invalid_mechanics";
  case TerminalClassification::reference_reproduction_failure:
    return "reference_reproduction_failure";
  case TerminalClassification::encoder_boundary_not_reproduced:
    return "encoder_boundary_not_reproduced";
  case TerminalClassification::domain_separation_sufficient:
    return "domain_separation_sufficient";
  case TerminalClassification::domain_scale_separation_sufficient:
    return "domain_scale_separation_sufficient";
  case TerminalClassification::coarse_position_separation_sufficient:
    return "coarse_position_separation_sufficient";
  case TerminalClassification::factors_restored_order_not_restored:
    return "factors_restored_order_not_restored";
  case TerminalClassification::order_restored_factors_not_restored:
    return "order_restored_factors_not_restored";
  case TerminalClassification::fixed_summaries_not_sufficient:
    return "fixed_summaries_not_sufficient";
  }
  return "invalid_mechanics";
}

struct GateResult {
  ContinuousResult encoder_minus_channel{};
  OrderResult encoder_order{};
  OrderResult channel_order{};
  std::array<CandidateResult, kCandidateCount> candidates{};
  bool numeric_inputs_valid{false};
  bool mechanics_valid{false};
  bool boundary_reproduced{false};
  TerminalClassification classification{
      TerminalClassification::invalid_mechanics};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool valid_interval(double point, double low,
                                         double high) {
  return finite(point) && finite(low) && finite(high) && low <= high;
}

[[nodiscard]] inline bool mechanics_valid(const ValidityInput &input) {
  return input.no_training_or_end_to_end &&
         input.capture_and_identity_exact &&
         input.parameters_and_rng_unchanged && input.partitions_valid &&
         input.projection_valid && input.deterministic_tables_valid;
}

[[nodiscard]] inline bool continuous_restored(
    ContinuousClassification classification) {
  return classification == ContinuousClassification::noninferior ||
         classification == ContinuousClassification::material_gain;
}

} // namespace detail

[[nodiscard]] inline ContinuousResult
evaluate_continuous(const ContinuousInput &input) {
  ContinuousResult result{};
  result.numeric_valid =
      detail::valid_interval(input.point, input.low, input.high) &&
      std::all_of(input.seed_deltas.begin(), input.seed_deltas.end(),
                  detail::finite) &&
      std::all_of(input.family_deltas.begin(), input.family_deltas.end(),
                  detail::finite);
  if (!result.numeric_valid) {
    return result;
  }

  result.positive_seed_count = static_cast<int64_t>(std::count_if(
      input.seed_deltas.begin(), input.seed_deltas.end(),
      [](double value) { return value > 0.0; }));
  result.noninferior_seed_count = static_cast<int64_t>(std::count_if(
      input.seed_deltas.begin(), input.seed_deltas.end(),
      [](double value) { return value > kNoninferiorityMargin; }));
  const bool families_noninferior = std::all_of(
      input.family_deltas.begin(), input.family_deltas.end(),
      [](double value) { return value > kFamilyNoninferiorityMargin; });

  result.material_gain = input.point >= kMaterialityThreshold &&
                         input.low > 0.0 &&
                         result.positive_seed_count >= 2;
  result.noninferior = input.low > kNoninferiorityMargin &&
                       result.noninferior_seed_count >= 2 &&
                       families_noninferior;
  if (result.material_gain) {
    result.classification = ContinuousClassification::material_gain;
  } else if (result.noninferior) {
    result.classification = ContinuousClassification::noninferior;
  } else {
    result.classification = ContinuousClassification::unresolved;
  }
  return result;
}

[[nodiscard]] inline OrderResult evaluate_order(const OrderInput &input) {
  OrderResult result{};
  result.numeric_valid =
      detail::valid_interval(input.point, input.low, input.high) &&
      std::all_of(input.seed_points.begin(), input.seed_points.end(),
                  detail::finite);
  if (!result.numeric_valid) {
    return result;
  }
  result.above_chance_seed_count = static_cast<int64_t>(std::count_if(
      input.seed_points.begin(), input.seed_points.end(),
      [](double value) { return value > kOrderChance; }));
  if (input.point >= kOrderDecodablePoint && input.low > kOrderChance &&
      result.above_chance_seed_count >= 2) {
    result.classification = OrderClassification::order_decodable;
  } else if (input.high <= 0.55) {
    result.classification = OrderClassification::chance_consistent;
  } else {
    result.classification = OrderClassification::order_unresolved;
  }
  return result;
}

[[nodiscard]] inline GateResult evaluate(const GateInput &input) {
  GateResult result{};
  result.encoder_minus_channel =
      evaluate_continuous(input.encoder_minus_channel);
  result.encoder_order = evaluate_order(input.encoder_order);
  result.channel_order = evaluate_order(input.channel_order);
  result.numeric_inputs_valid = result.encoder_minus_channel.numeric_valid &&
                                result.encoder_order.numeric_valid &&
                                result.channel_order.numeric_valid;
  for (std::size_t index = 0; index < kCandidateCount; ++index) {
    auto &candidate = result.candidates[index];
    candidate.minus_encoder =
        evaluate_continuous(input.candidates[index].minus_encoder);
    candidate.minus_channel =
        evaluate_continuous(input.candidates[index].minus_channel);
    candidate.order = evaluate_order(input.candidates[index].order);
    result.numeric_inputs_valid =
        result.numeric_inputs_valid && candidate.minus_encoder.numeric_valid &&
        candidate.minus_channel.numeric_valid && candidate.order.numeric_valid;
    candidate.continuous_restored =
        detail::continuous_restored(candidate.minus_encoder.classification) &&
        candidate.minus_channel.classification ==
            ContinuousClassification::material_gain;
    candidate.restored =
        candidate.continuous_restored &&
        candidate.order.classification == OrderClassification::order_decodable;
  }

  result.mechanics_valid = detail::mechanics_valid(input.validity);
  if (!result.numeric_inputs_valid || !result.mechanics_valid) {
    result.classification = TerminalClassification::invalid_mechanics;
    return result;
  }
  if (!input.validity.references_reproduced) {
    result.classification =
        TerminalClassification::reference_reproduction_failure;
    return result;
  }
  if (!input.validity.shuffled_controls_pass) {
    result.classification = TerminalClassification::invalid_mechanics;
    return result;
  }

  result.boundary_reproduced =
      result.encoder_minus_channel.classification ==
          ContinuousClassification::material_gain &&
      result.encoder_order.classification ==
          OrderClassification::order_decodable &&
      result.channel_order.classification !=
          OrderClassification::order_decodable;
  if (!result.boundary_reproduced) {
    result.classification =
        TerminalClassification::encoder_boundary_not_reproduced;
    return result;
  }

  constexpr std::array<TerminalClassification, kCandidateCount>
      restored_classifications{
          TerminalClassification::domain_separation_sufficient,
          TerminalClassification::domain_scale_separation_sufficient,
          TerminalClassification::coarse_position_separation_sufficient};
  for (std::size_t index = 0; index < kCandidateCount; ++index) {
    if (result.candidates[index].restored) {
      result.classification = restored_classifications[index];
      return result;
    }
  }

  const bool any_continuous =
      std::any_of(result.candidates.begin(), result.candidates.end(),
                  [](const CandidateResult &candidate) {
                    return candidate.continuous_restored;
                  });
  const bool any_order =
      std::any_of(result.candidates.begin(), result.candidates.end(),
                  [](const CandidateResult &candidate) {
                    return candidate.order.classification ==
                           OrderClassification::order_decodable;
                  });
  if (any_continuous) {
    result.classification =
        TerminalClassification::factors_restored_order_not_restored;
  } else if (any_order) {
    result.classification =
        TerminalClassification::order_restored_factors_not_restored;
  } else {
    result.classification =
        TerminalClassification::fixed_summaries_not_sufficient;
  }
  return result;
}

} // namespace cuwacunu::tests::pooling_structure_mechanism_map_gate
