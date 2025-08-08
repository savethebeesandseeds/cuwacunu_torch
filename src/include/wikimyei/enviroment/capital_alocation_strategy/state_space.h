/* state_space.h */
#pragma once

#include <torch/torch.h>
#include "piaabo/dutils.h"

RUNTIME_WARNING("(state_space.h)[] does not have a mask to follow the features, but this is ok.\n");

namespace cuwacunu {
namespace wikimyei {
namespace capital_alocation_strategy {

/* state_space_t */
using state_features_t = torch::Tensor;
struct state_space_t {
  instrument_v_t<state_features_t> instruments_state_feat; /* state features per instrument */
  /* Feature Constructor */
  state_space_t(const instrument_v_t<state_features_t> instruments_state_feat);
  /* Copy Constructor */
  state_space_t(const state_space_t& src);
  /* Unpack method to convert vector of tensors into a single tensor */
  torch::Tensor unpack() const;
};

} /* namespace capital_alocation_strategy */
} /* namespace wikimyei */
} /* namespace cuwacunu */