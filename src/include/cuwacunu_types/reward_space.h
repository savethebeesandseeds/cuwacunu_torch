#pragma once
#include "dutils.h"

namespace cuwacunu {
/* reward_space_t */
using reward_feature_t = float;
struct reward_space_t {
  instrument_v_t<reward_feature_t> instrument_reward; /* rewatd per instrument */
  float evaluate_reward() const;
  /* Constructor declaration */
  reward_space_t(instrument_v_t<reward_feature_t> instrument_reward);
};
} /* namespace cuwacunu */