#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace cuwacunu::tests::mtf_surface_sufficiency_map_gate {

constexpr std::size_t kSeedCount = 3;
constexpr std::size_t kFamilyCount = 4;
constexpr double kMaterialityThreshold = 0.02;
constexpr double kNoninferiorityMargin = -0.02;
constexpr double kFamilyNoninferiorityMargin = -0.05;

using SeedDeltas = std::array<double, kSeedCount>;
using FamilyDeltas = std::array<double, kFamilyCount>;

struct TransitionInput {
  double point{0.0};
  double low{0.0};
  double high{0.0};
  SeedDeltas seed_deltas{};
  FamilyDeltas family_deltas{};
};

enum class TransitionClassification {
  invalid_numeric,
  material_loss,
  family_specific_loss,
  noninferior,
  material_gain,
  unresolved,
};

[[nodiscard]] inline const char *
transition_classification_name(TransitionClassification classification) {
  switch (classification) {
  case TransitionClassification::invalid_numeric:
    return "invalid_numeric";
  case TransitionClassification::material_loss:
    return "material_loss";
  case TransitionClassification::family_specific_loss:
    return "family_specific_loss";
  case TransitionClassification::noninferior:
    return "noninferior";
  case TransitionClassification::material_gain:
    return "material_gain";
  case TransitionClassification::unresolved:
    return "unresolved";
  }
  return "invalid_numeric";
}

struct TransitionResult {
  bool numeric_valid{false};
  int64_t negative_seed_count{0};
  int64_t noninferior_seed_count{0};
  int64_t positive_seed_count{0};
  int64_t negative_family_count{0};
  std::array<bool, kFamilyCount> family_noninferiority_pass{};
  bool macro_loss_point_pass{false};
  bool macro_loss_interval_pass{false};
  bool macro_loss_seed_pass{false};
  bool macro_loss{false};
  bool material_loss{false};
  bool family_specific_loss{false};
  bool noninferiority_interval_pass{false};
  bool noninferiority_seed_pass{false};
  bool all_families_noninferior{false};
  bool noninferior{false};
  bool material_gain_point_pass{false};
  bool material_gain_interval_pass{false};
  bool material_gain_seed_pass{false};
  bool material_gain{false};
  TransitionClassification classification{
      TransitionClassification::invalid_numeric};
};

struct TrackInput {
  TransitionInput tokenizer_minus_raw{};
  TransitionInput encoder_minus_tokenizer{};
  TransitionInput served_minus_encoder{};
  TransitionInput served_minus_raw{};
};

enum class AdjacentStage {
  none,
  token_construction,
  encoder_processing,
  serving_pooling,
};

[[nodiscard]] inline const char *stage_name(AdjacentStage stage) {
  switch (stage) {
  case AdjacentStage::none:
    return "none";
  case AdjacentStage::token_construction:
    return "token_construction";
  case AdjacentStage::encoder_processing:
    return "encoder_processing";
  case AdjacentStage::serving_pooling:
    return "serving_pooling";
  }
  return "none";
}

struct TrackResult {
  TransitionResult tokenizer_minus_raw{};
  TransitionResult encoder_minus_tokenizer{};
  TransitionResult served_minus_encoder{};
  TransitionResult served_minus_raw{};
  AdjacentStage earliest_adjacent_loss{AdjacentStage::none};
};

struct ValidityInput {
  bool no_optimizer_or_backward{false};
  bool parameters_and_rng_unchanged{false};
  bool repeated_capture_identical{false};
  bool surface_identity_hashes_match{false};
  bool projections_valid{false};
  bool accepted_reference_reproduced{false};
  bool legacy_raw_reference_reproduced{false};
  bool tokenizer_plan_reproduced{false};
  bool normalized_fixed96_informative{false};
  bool shuffled_controls_pass{false};
};

struct GateInput {
  TrackInput native{};
  TrackInput fixed96{};
  TransitionInput legacy_served_minus_raw{};
  ValidityInput validity{};
};

enum class TerminalClassification {
  invalid_numeric_or_mechanics,
  accepted_step_zero_reference_not_reproduced,
  legacy_raw_reference_not_reproduced,
  tokenizer_plan_not_reproduced,
  normalized_raw_control_not_informative,
  shuffled_target_leakage,
  token_construction_loss,
  token_construction_family_specific_loss,
  encoder_processing_loss,
  encoder_processing_family_specific_loss,
  serving_pooling_loss,
  serving_pooling_family_specific_loss,
  prepool_width_advantage_only,
  projection_sensitive_localization,
  distributed_internal_loss,
  legacy_raw_gap_normalization_confounded,
  no_material_surface_gap_reproduced,
  no_terminal_interpretation_supported,
};

[[nodiscard]] inline const char *
terminal_classification_name(TerminalClassification classification) {
  switch (classification) {
  case TerminalClassification::invalid_numeric_or_mechanics:
    return "invalid_numeric_or_mechanics";
  case TerminalClassification::accepted_step_zero_reference_not_reproduced:
    return "accepted_step_zero_reference_not_reproduced";
  case TerminalClassification::legacy_raw_reference_not_reproduced:
    return "legacy_raw_reference_not_reproduced";
  case TerminalClassification::tokenizer_plan_not_reproduced:
    return "tokenizer_plan_not_reproduced";
  case TerminalClassification::normalized_raw_control_not_informative:
    return "normalized_raw_control_not_informative";
  case TerminalClassification::shuffled_target_leakage:
    return "shuffled_target_leakage";
  case TerminalClassification::token_construction_loss:
    return "token_construction_loss";
  case TerminalClassification::token_construction_family_specific_loss:
    return "token_construction_family_specific_loss";
  case TerminalClassification::encoder_processing_loss:
    return "encoder_processing_loss";
  case TerminalClassification::encoder_processing_family_specific_loss:
    return "encoder_processing_family_specific_loss";
  case TerminalClassification::serving_pooling_loss:
    return "serving_pooling_loss";
  case TerminalClassification::serving_pooling_family_specific_loss:
    return "serving_pooling_family_specific_loss";
  case TerminalClassification::prepool_width_advantage_only:
    return "prepool_width_advantage_only";
  case TerminalClassification::projection_sensitive_localization:
    return "projection_sensitive_localization";
  case TerminalClassification::distributed_internal_loss:
    return "distributed_internal_loss";
  case TerminalClassification::legacy_raw_gap_normalization_confounded:
    return "legacy_raw_gap_normalization_confounded";
  case TerminalClassification::no_material_surface_gap_reproduced:
    return "no_material_surface_gap_reproduced";
  case TerminalClassification::no_terminal_interpretation_supported:
    return "no_terminal_interpretation_supported";
  }
  return "invalid_numeric_or_mechanics";
}

struct GateResult {
  TrackResult native{};
  TrackResult fixed96{};
  TransitionResult legacy_served_minus_raw{};
  bool numeric_inputs_valid{false};
  bool validity_controls_pass{false};
  TerminalClassification classification{
      TerminalClassification::invalid_numeric_or_mechanics};
};

namespace detail {

[[nodiscard]] inline bool finite(double value) { return std::isfinite(value); }

[[nodiscard]] inline bool valid_transition(const TransitionInput &input) {
  const bool interval_valid = finite(input.point) && finite(input.low) &&
                              finite(input.high) && input.low <= input.point &&
                              input.point <= input.high;
  const bool seeds_valid =
      std::all_of(input.seed_deltas.begin(), input.seed_deltas.end(), finite);
  const bool families_valid = std::all_of(input.family_deltas.begin(),
                                          input.family_deltas.end(), finite);
  return interval_valid && seeds_valid && families_valid;
}

[[nodiscard]] inline bool
is_loss_classification(TransitionClassification classification) {
  return classification == TransitionClassification::material_loss ||
         classification == TransitionClassification::family_specific_loss;
}

[[nodiscard]] inline bool mechanics_controls_pass(const ValidityInput &input) {
  return input.no_optimizer_or_backward && input.parameters_and_rng_unchanged &&
         input.repeated_capture_identical &&
         input.surface_identity_hashes_match && input.projections_valid;
}

[[nodiscard]] inline bool validity_controls_pass(const ValidityInput &input) {
  return mechanics_controls_pass(input) &&
         input.accepted_reference_reproduced &&
         input.legacy_raw_reference_reproduced &&
         input.tokenizer_plan_reproduced &&
         input.normalized_fixed96_informative && input.shuffled_controls_pass;
}

[[nodiscard]] inline const TransitionResult &
transition_at(const TrackResult &track, AdjacentStage stage) {
  switch (stage) {
  case AdjacentStage::token_construction:
    return track.tokenizer_minus_raw;
  case AdjacentStage::encoder_processing:
    return track.encoder_minus_tokenizer;
  case AdjacentStage::serving_pooling:
    return track.served_minus_encoder;
  case AdjacentStage::none:
    return track.served_minus_raw;
  }
  return track.served_minus_raw;
}

[[nodiscard]] inline TerminalClassification
aligned_stage_classification(AdjacentStage stage, bool both_material) {
  switch (stage) {
  case AdjacentStage::token_construction:
    return both_material ? TerminalClassification::token_construction_loss
                         : TerminalClassification::
                               token_construction_family_specific_loss;
  case AdjacentStage::encoder_processing:
    return both_material ? TerminalClassification::encoder_processing_loss
                         : TerminalClassification::
                               encoder_processing_family_specific_loss;
  case AdjacentStage::serving_pooling:
    return both_material
               ? TerminalClassification::serving_pooling_loss
               : TerminalClassification::serving_pooling_family_specific_loss;
  case AdjacentStage::none:
    return TerminalClassification::no_terminal_interpretation_supported;
  }
  return TerminalClassification::no_terminal_interpretation_supported;
}

} // namespace detail

[[nodiscard]] inline bool
is_loss_classification(TransitionClassification classification) {
  return detail::is_loss_classification(classification);
}

[[nodiscard]] inline TransitionResult
evaluate_transition(const TransitionInput &input) {
  TransitionResult result{};
  result.numeric_valid = detail::valid_transition(input);
  if (!result.numeric_valid) {
    return result;
  }

  for (double delta : input.seed_deltas) {
    result.negative_seed_count += delta < 0.0 ? 1 : 0;
    result.noninferior_seed_count += delta > kNoninferiorityMargin ? 1 : 0;
    result.positive_seed_count += delta > 0.0 ? 1 : 0;
  }
  result.all_families_noninferior = true;
  for (std::size_t family = 0; family < kFamilyCount; ++family) {
    const double delta = input.family_deltas[family];
    result.negative_family_count += delta < 0.0 ? 1 : 0;
    result.family_noninferiority_pass[family] =
        delta > kFamilyNoninferiorityMargin;
    result.all_families_noninferior = result.all_families_noninferior &&
                                      result.family_noninferiority_pass[family];
  }

  result.macro_loss_point_pass = input.point <= -kMaterialityThreshold;
  result.macro_loss_interval_pass = input.high < 0.0;
  result.macro_loss_seed_pass = result.negative_seed_count >= 2;
  result.macro_loss = result.macro_loss_point_pass &&
                      result.macro_loss_interval_pass &&
                      result.macro_loss_seed_pass;
  result.material_loss = result.macro_loss && result.negative_family_count >= 2;
  result.family_specific_loss =
      result.macro_loss && result.negative_family_count == 1;

  result.noninferiority_interval_pass = input.low > kNoninferiorityMargin;
  result.noninferiority_seed_pass = result.noninferior_seed_count >= 2;
  result.noninferior = result.noninferiority_interval_pass &&
                       result.noninferiority_seed_pass &&
                       result.all_families_noninferior;

  result.material_gain_point_pass = input.point >= kMaterialityThreshold;
  result.material_gain_interval_pass = input.low > 0.0;
  result.material_gain_seed_pass = result.positive_seed_count >= 2;
  result.material_gain = result.material_gain_point_pass &&
                         result.material_gain_interval_pass &&
                         result.material_gain_seed_pass;

  if (result.material_loss) {
    result.classification = TransitionClassification::material_loss;
  } else if (result.family_specific_loss) {
    result.classification = TransitionClassification::family_specific_loss;
  } else if (result.material_gain) {
    result.classification = TransitionClassification::material_gain;
  } else if (result.noninferior) {
    result.classification = TransitionClassification::noninferior;
  } else {
    result.classification = TransitionClassification::unresolved;
  }
  return result;
}

[[nodiscard]] inline TrackResult evaluate_track(const TrackInput &input) {
  TrackResult result{};
  result.tokenizer_minus_raw = evaluate_transition(input.tokenizer_minus_raw);
  result.encoder_minus_tokenizer =
      evaluate_transition(input.encoder_minus_tokenizer);
  result.served_minus_encoder = evaluate_transition(input.served_minus_encoder);
  result.served_minus_raw = evaluate_transition(input.served_minus_raw);
  if (detail::is_loss_classification(
          result.tokenizer_minus_raw.classification)) {
    result.earliest_adjacent_loss = AdjacentStage::token_construction;
  } else if (detail::is_loss_classification(
                 result.encoder_minus_tokenizer.classification)) {
    result.earliest_adjacent_loss = AdjacentStage::encoder_processing;
  } else if (detail::is_loss_classification(
                 result.served_minus_encoder.classification)) {
    result.earliest_adjacent_loss = AdjacentStage::serving_pooling;
  }
  return result;
}

[[nodiscard]] inline GateResult evaluate(const GateInput &input) {
  GateResult result{};
  result.native = evaluate_track(input.native);
  result.fixed96 = evaluate_track(input.fixed96);
  result.legacy_served_minus_raw =
      evaluate_transition(input.legacy_served_minus_raw);
  result.numeric_inputs_valid =
      result.native.tokenizer_minus_raw.numeric_valid &&
      result.native.encoder_minus_tokenizer.numeric_valid &&
      result.native.served_minus_encoder.numeric_valid &&
      result.native.served_minus_raw.numeric_valid &&
      result.fixed96.tokenizer_minus_raw.numeric_valid &&
      result.fixed96.encoder_minus_tokenizer.numeric_valid &&
      result.fixed96.served_minus_encoder.numeric_valid &&
      result.fixed96.served_minus_raw.numeric_valid &&
      result.legacy_served_minus_raw.numeric_valid;
  result.validity_controls_pass =
      detail::validity_controls_pass(input.validity);

  if (!result.numeric_inputs_valid ||
      !detail::mechanics_controls_pass(input.validity)) {
    result.classification =
        TerminalClassification::invalid_numeric_or_mechanics;
    return result;
  }
  if (!input.validity.accepted_reference_reproduced) {
    result.classification =
        TerminalClassification::accepted_step_zero_reference_not_reproduced;
    return result;
  }
  if (!input.validity.legacy_raw_reference_reproduced) {
    result.classification =
        TerminalClassification::legacy_raw_reference_not_reproduced;
    return result;
  }
  if (!input.validity.tokenizer_plan_reproduced) {
    result.classification =
        TerminalClassification::tokenizer_plan_not_reproduced;
    return result;
  }
  if (!input.validity.normalized_fixed96_informative) {
    result.classification =
        TerminalClassification::normalized_raw_control_not_informative;
    return result;
  }
  if (!input.validity.shuffled_controls_pass) {
    result.classification = TerminalClassification::shuffled_target_leakage;
    return result;
  }

  const AdjacentStage native_stage = result.native.earliest_adjacent_loss;
  const AdjacentStage fixed_stage = result.fixed96.earliest_adjacent_loss;
  const bool fixed_total_loss = detail::is_loss_classification(
      result.fixed96.served_minus_raw.classification);
  if (!fixed_total_loss) {
    if (detail::is_loss_classification(
            result.legacy_served_minus_raw.classification)) {
      result.classification =
          TerminalClassification::legacy_raw_gap_normalization_confounded;
    } else if (detail::is_loss_classification(
                   result.native.served_minus_raw.classification)) {
      result.classification =
          TerminalClassification::projection_sensitive_localization;
    } else {
      result.classification =
          TerminalClassification::no_material_surface_gap_reproduced;
    }
    return result;
  }

  if (native_stage != AdjacentStage::none && native_stage == fixed_stage) {
    const bool both_material =
        detail::transition_at(result.native, native_stage).classification ==
            TransitionClassification::material_loss &&
        detail::transition_at(result.fixed96, fixed_stage).classification ==
            TransitionClassification::material_loss;
    result.classification =
        detail::aligned_stage_classification(native_stage, both_material);
  } else if (native_stage == AdjacentStage::serving_pooling &&
             fixed_stage == AdjacentStage::none &&
             result.fixed96.served_minus_encoder.classification ==
                 TransitionClassification::noninferior) {
    result.classification =
        TerminalClassification::prepool_width_advantage_only;
  } else if (native_stage != fixed_stage) {
    result.classification =
        TerminalClassification::projection_sensitive_localization;
  } else if (native_stage == AdjacentStage::none &&
             fixed_stage == AdjacentStage::none) {
    result.classification = TerminalClassification::distributed_internal_loss;
  } else {
    result.classification =
        TerminalClassification::no_terminal_interpretation_supported;
  }
  return result;
}

} // namespace cuwacunu::tests::mtf_surface_sufficiency_map_gate
