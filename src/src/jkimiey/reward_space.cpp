#include "piaabo/dutils.h"
#include "iinuji/reward_space.h"

RUNTIME_WARNING("(reward_space.cpp)[reward_space_t::reward_space_t] #FIXME Reward == 1.0.\n");
RUNTIME_WARNING("(reward_space.cpp)[] consider comparing against AMPL (A Mathematical Programming Language) results.\n");

namespace cuwacunu {
/* reward_space_t */
reward_space_t::reward_space_t(instrument_v_t<reward_feature_t> instrument_reward) 
  : instrument_reward(instrument_reward) {}
float reward_space_t::evaluate_reward() const {
  /* retrive the actual reward from reward_space_t */
  return 1.0;
}
} /* namespace cuwacunu */