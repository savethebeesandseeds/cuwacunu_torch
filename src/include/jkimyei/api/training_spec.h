// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::jkimyei::training {

enum class training_task_t {
  vicreg_node_representation,
  mdn_expected_value_inference,
};

enum class training_optimizer_t {
  adam,
};

struct training_run_spec_t {
  std::string version_token{"jkimyei.inference.expected_value.mdn.v1"};
  std::string training_id{};
  training_task_t task{training_task_t::mdn_expected_value_inference};
  std::string component_id{};
  training_optimizer_t optimizer{training_optimizer_t::adam};
  double learning_rate{0.0};
  int64_t max_steps{0};
  int64_t batch_size{0};
  double grad_clip_norm{0.0};
  int64_t checkpoint_every{0};
  int64_t report_every{1};
  int64_t validation_every{0};
  int64_t seed{0};
  bool freeze_representation{true};
};

namespace training_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline training_task_t parse_task(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "vicreg_node_representation") {
    return training_task_t::vicreg_node_representation;
  }
  if (value == "mdn_expected_value_inference") {
    return training_task_t::mdn_expected_value_inference;
  }
  throw std::runtime_error("[training_spec] invalid TASK: " + value);
}

[[nodiscard]] inline training_optimizer_t parse_optimizer(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "adam") {
    return training_optimizer_t::adam;
  }
  throw std::runtime_error("[training_spec] invalid OPTIMIZER: " + value);
}

[[nodiscard]] inline const char *expected_version_token(training_task_t task) {
  switch (task) {
  case training_task_t::vicreg_node_representation:
    return "jkimyei.representation.vicreg.v1";
  case training_task_t::mdn_expected_value_inference:
    return "jkimyei.inference.expected_value.mdn.v1";
  }
  throw std::runtime_error("[training_spec] unknown training task");
}

} // namespace training_spec_detail

inline void validate_training_run_spec(const training_run_spec_t &spec) {
  const std::string expected =
      training_spec_detail::expected_version_token(spec.task);
  if (spec.version_token != expected) {
    throw std::runtime_error(
        "[training_spec] unsupported version token for task");
  }
  if (spec.training_id.empty()) {
    throw std::runtime_error("[training_spec] training_id is required");
  }
  if (spec.component_id.empty()) {
    throw std::runtime_error("[training_spec] component_id is required");
  }
  if (spec.optimizer != training_optimizer_t::adam) {
    throw std::runtime_error("[training_spec] v1 supports adam only");
  }
  if (spec.learning_rate <= 0.0) {
    throw std::runtime_error("[training_spec] learning_rate must be > 0");
  }
  if (spec.max_steps < 0 || spec.batch_size <= 0 || spec.grad_clip_norm < 0.0 ||
      spec.checkpoint_every < 0 || spec.report_every <= 0 ||
      spec.validation_every < 0 || spec.seed < 0) {
    throw std::runtime_error(
        "[training_spec] invalid training cadence/options");
  }
  if (spec.validation_every != 0) {
    throw std::runtime_error(
        "[training_spec] VALIDATION_EVERY is not implemented in v1; set it "
        "to 0");
  }
  if (spec.task == training_task_t::mdn_expected_value_inference &&
      !spec.freeze_representation) {
    throw std::runtime_error(
        "[training_spec] v1 MDN ExpectedValue training requires frozen "
        "representation");
  }
}

[[nodiscard]] inline training_run_spec_t
decode_training_run_spec_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "TRAINING");
  training_run_spec_t spec{};
  spec.version_token = kv::optional(block, "VERSION", spec.version_token);
  spec.training_id = kv::required(block, "TRAINING_ID");
  spec.task = training_spec_detail::parse_task(kv::required(block, "TASK"));
  spec.component_id = kv::required(block, "COMPONENT_ID");
  spec.optimizer = training_spec_detail::parse_optimizer(
      kv::optional(block, "OPTIMIZER", "adam"));
  spec.learning_rate = kv::parse_double(kv::required(block, "LEARNING_RATE"));
  spec.max_steps = kv::parse_i64(kv::optional(block, "MAX_STEPS", "0"));
  spec.batch_size = kv::parse_i64(kv::required(block, "BATCH_SIZE"));
  spec.grad_clip_norm =
      kv::parse_double(kv::optional(block, "GRAD_CLIP_NORM", "0.0"));
  spec.checkpoint_every =
      kv::parse_i64(kv::optional(block, "CHECKPOINT_EVERY", "0"));
  spec.report_every = kv::parse_i64(kv::optional(block, "REPORT_EVERY", "1"));
  spec.validation_every =
      kv::parse_i64(kv::optional(block, "VALIDATION_EVERY", "0"));
  spec.seed = kv::parse_i64(kv::optional(block, "SEED", "0"));
  spec.freeze_representation =
      kv::parse_bool(kv::optional(block, "FREEZE_REPRESENTATION", "true"));
  validate_training_run_spec(spec);
  return spec;
}

} // namespace cuwacunu::jkimyei::training
