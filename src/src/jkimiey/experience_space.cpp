#include "piaabo/dutils.h"
#include "iinuji/experience_space.h"

namespace cuwacunu {
/* experience_space_t */
experience_space_t::experience_space_t(
  const state_space_t& state, 
  const action_space_t& action, 
  const state_space_t& next_state, 
  const reward_space_t& reward, 
  bool done, 
  const learn_space_t& learn)
  : state(state),
    action(action),
    next_state(next_state),
    reward(reward),
    done(done),
    learn(learn) {}
} /* namespace cuwacunu */
