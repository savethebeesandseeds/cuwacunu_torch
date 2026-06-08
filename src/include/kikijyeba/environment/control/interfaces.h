// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include "kikijyeba/environment/control/types.h"

namespace cuwacunu::kikijyeba::environment {

class world_iface_t {
public:
  virtual ~world_iface_t() = default;

  [[nodiscard]] virtual observation_t reset(const episode_spec_t &spec) = 0;
  [[nodiscard]] virtual transition_t step(const action_t &action) = 0;
};

class policy_adapter_iface_t {
public:
  virtual ~policy_adapter_iface_t() = default;

  [[nodiscard]] virtual std::string policy_id() const = 0;
  [[nodiscard]] virtual policy_kind_t policy_kind() const = 0;
  [[nodiscard]] virtual action_t act(const observation_t &observation) = 0;
  [[nodiscard]] virtual action_t
  collect_action(const observation_t &observation) {
    return act(observation);
  }
};

} // namespace cuwacunu::kikijyeba::environment
