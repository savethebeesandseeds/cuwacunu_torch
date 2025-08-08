/* wk_enviroment.h */
#pragma once

namespace cuwacunu {
namespace wikimyei {

struct experience_t {
  state_t  state;   // s_{t+1}
  reward_t reward;  // r_{t+1}
  action_t action;  // a_{t}  (handy for replay)
  bool     done;    // episode finished?
  info_t   info{};  // diagnostics
};

/**
 * Generic Gym-style environment.
 *  State, Action can be anything: Tensor, struct, vector
 */
template<
  typename state_t,
  typename action_t,
  typename reward_t,
  typename info_t
>
class Environment {
public:

  virtual ~Environment() = default;

  /* Reset to first timestep; return initial state. */
  virtual state_t reset() = 0;

  /* Advance one step; mutates internal state. */
  virtual experience_t step(const action_t& a) = 0;

  /* Is the current episode finished? */
  virtual bool is_done() const = 0;

  /* Peek at the latest state without stepping. */
  virtual const state_t& current_state() const = 0;
};

} /* namespace wikimyei */
} /* namespace cuwacunu */